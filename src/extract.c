#define _POSIX_C_SOURCE 200809L
#include "extract.h"
#include "cli.h"
#include "extras.h"
#include "file_entry_t.h"
#include <jpeglib.h>
#include <png.h>
#include <regex.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <zip.h>
#include <zipconf.h>

/// the file names must follow [<number>]_<name>.cbz
/// patern will get the <number> portion
int32_t extract_file_name_number(const cli_flags_t *cli_flags,
                                 const char        *filename) {
  regex_t     regex;
  regmatch_t  group_array[2];
  const char *pattern = "\\[(.*?)\\]";
  printfv(*cli_flags, DARK_ORANGE, "Trying to find %s in %s\n", pattern,
          filename);
  regcomp(&regex, pattern, REG_EXTENDED);

  if (regexec(&regex, filename, 2, group_array, 0) != 0) {
    printfv(*cli_flags, RED, "Regex failed\n");
    regfree(&regex);
    return -1; // handle error
  }

  int    start = group_array[1].rm_so;
  int    end   = group_array[1].rm_eo;
  size_t size  = end - start;
  char   number[size + 1];

  strncpy(number, filename + start, size);
  number[size] = 0;
  regfree(&regex);

  return atoi(number);
}

bool is_photo(const char *filename) {
  char *ext = strrchr(filename, '.');
  if (!ext || ext == filename) {
    return false;
  }
  return strcasecmp(ext, ".png") == 0 || strcasecmp(ext, ".jpg") == 0 ||
         strcasecmp(ext, ".jpeg") == 0;
}

bool is_png(const unsigned char *data, size_t size) {
  if (size < 8) {
    return false;
  }
  return png_sig_cmp(data, 0, 8) == 0;
}

bool is_jpeg(const unsigned char *data, size_t size) {
  if (size < 2) {
    return false;
  }
  return data[0] == 0xFF && data[1] == 0xD8;
}

void _png_memory_read(const cli_flags_t *cli_flags, png_structp png_ptr,
                      png_bytep data, png_size_t length) {
  memory_reader_state_t *state =
      (memory_reader_state_t *)png_get_io_ptr(png_ptr);
  if (state->offset + length <= state->size) {
    memcpyv(*cli_flags, data, state->data + state->offset, length, sizeof(data),
            "data");
    state->offset += length;
  } else {
    png_error(png_ptr, "Read error in _png_memory_read");
  }
}

void _get_png_dimensions_from_memory(const cli_flags_t   *cli_flags,
                                     const unsigned char *data, size_t size,
                                     uint32_t *width, uint32_t *height) {
  png_image image;
  memset(&image, 0, sizeof(image));
  image.version = PNG_IMAGE_VERSION;
  image.opaque  = NULL;

  if (!png_image_begin_read_from_memory(&image, data, size)) {
    printfv(*cli_flags, RED, "Error processing PNG image\n");
    return;
  }
  *width  = image.width;
  *height = image.height;

  png_image_free(&image);
}

void _get_jpeg_dimensions_from_memory(const cli_flags_t   *cli_flags,
                                      const unsigned char *data, size_t size,
                                      uint32_t *width, uint32_t *height) {
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr         jerr;

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  jpeg_mem_src(&cinfo, (unsigned char *)data, size);
  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  *width  = cinfo.output_width;
  *height = cinfo.output_height;

  while (cinfo.output_scanline < cinfo.output_height) {
    unsigned char *row_pointer = (unsigned char *)mallocv(
        *cli_flags, "row_pointer",
        (size_t)(cinfo.output_width * cinfo.output_components), -1);
    jpeg_read_scanlines(&cinfo, &row_pointer, 1);
    freev(*cli_flags, row_pointer, "row_pointer", -1);
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
}
bool _get_width_height_and_type(const cli_flags_t *cli_flags, uint32_t *width,
                                uint32_t *height, char **ext,
                                const unsigned char *contents,
                                struct zip_stat     *st) {
  // Note: cannot use filetype variable since it might be named as a png
  // but be a jpg
  if (is_png((unsigned char *)contents, st->size)) {
    printfv(*cli_flags, DARK_YELLOW, "File is detected as a PNG\n");
    _get_png_dimensions_from_memory(cli_flags, (unsigned char *)contents,
                                    st->size, width, height);
    strncpy(*ext, ".png\0", 5);
    return true;
  }
  if (is_jpeg((unsigned char *)contents, st->size)) {
    printfv(*cli_flags, DARK_YELLOW, "File is detected as a JPEG\n");
    _get_jpeg_dimensions_from_memory(cli_flags, (unsigned char *)contents,
                                     st->size, width, height);
    strncpy(*ext, ".jpg\0", 5);
    return true;
  }
  return false;
}

void _find_rotated_and_double_pages(const cli_flags_t *cli_flags,
                                    uint32_t *photo_counter, photo_t **photos) {
  uint32_t tmp_count = *photo_counter;
  for (uint32_t i = 0; i < tmp_count; ++i) {
    photo_t curr_photo = (*photos)[i];
    if (curr_photo.double_page && curr_photo.on_its_side) {
      if (curr_photo.width * 2 >= curr_photo.height) {
        curr_photo.on_its_side = false;
        printfv(*cli_flags, "", "%s is a double page\n", curr_photo.name);
      } else {
        curr_photo.double_page = DOUBLE_PAGE_FALSE;
        printfv(*cli_flags, "", "%s is on its side\n", curr_photo.name);
      }
      (*photo_counter)++;
    }
  }
}

void _handle_cbz_entry(const cli_flags_t *cli_flags, const char *cbz_path,
                       uint32_t *photo_counter, photo_t **photos,
                       uint32_t *photos_arr_len) {
  // Open the zip file
  int    err;
  zip_t *dest = zip_open(cbz_path, 0, &err);
  if (!dest) {
    fprintf(stderr, "Failed to open CBZ file: %s\n", cbz_path);
    return;
  }

  // Get the number of entries in the zip file
  zip_int64_t num_entries = zip_get_num_entries(dest, 0);

  for (zip_uint64_t i = 0; i < num_entries; i++) {
    // Get file stat for entry
    struct zip_stat st;
    zip_stat_init(&st);
    zip_stat_index(dest, i, 0, &st);

    // Extract file into memory
    zip_file_t *zf = zip_fopen_index(dest, i, 0);
    if (!zf) {
      continue; // Skip if cannot open file
    }

    unsigned char *contents = malloc(st.size);
    if (contents == NULL) {
      fprintf(stderr, "Failed to allocate memory\n");
      zip_fclose(zf);
      continue;
    }

    zip_fread(zf, contents, st.size);

    // Use contents to get width and height
    uint32_t width, height;
    char    *ext = mallocv(*cli_flags, "ext", sizeof(char) * 5, -1);
    strncpy(ext, "\0\0\0\0\0", 5);
    if (_get_width_height_and_type(cli_flags, &width, &height, &ext, contents,
                                   &st)) {
      // Check if we need to resize the photos array
      if (*photo_counter >= *photos_arr_len) {
        *photos_arr_len *= 2; // Double the size
        *photos          = realloc(*photos, *photos_arr_len * sizeof(photo_t));
        if (*photos == NULL) {
          fprintf(stderr, "Failed to reallocate memory\n");
          free(contents);
          zip_fclose(zf);
          break;
        }
      }

      // Update the photos array
      photo_t *photo     = &(*photos)[*photo_counter];
      photo->cbz_path    = strdup(cbz_path);
      photo->name        = strdup(st.name);
      photo->ext         = strdup(ext);
      photo->width       = width;
      photo->height      = height;
      photo->id          = *photo_counter;
      photo->on_its_side = false;
      photo->double_page = DOUBLE_PAGE_FALSE;

      (*photo_counter)++;
    }

    free(contents);
    free(ext);
    zip_fclose(zf);
  }

  zip_close(dest);
}

void adjust_for_double_pages(photo_t **photos, uint32_t *photo_count) {
  for (uint32_t i = 0; i < *photo_count; i++) {
    // Corrected condition for identifying a double page
    if ((*photos)[i].width > (*photos)[i].height) {
      // Increase the photo count to accommodate the new split page
      *photo_count += 1;
      *photos       = realloc(*photos, (*photo_count) * sizeof(photo_t));
      if (*photos == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
      }

      // Shift all subsequent photos one position to the right to make space
      for (uint32_t j = *photo_count - 1; j > i; j--) {
        (*photos)[j] = (*photos)[j - 1];
        // Duplicate the strings for the new right side of the double page to
        // prevent double free
        if (j == i + 1) { // Only for the immediate next photo, which is the new
                          // right side
          (*photos)[j].name = strdup((*photos)[j].name);
          if ((*photos)[j]
                  .cbz_path) { // If cbz_path is used, duplicate it as well
            (*photos)[j].cbz_path = strdup((*photos)[j].cbz_path);
          }
        }
      }

      // Update the double page status for both sides
      (*photos)[i].double_page     = DOUBLE_PAGE_FIRST;  // Left side
      (*photos)[i + 1].double_page = DOUBLE_PAGE_SECOND; // Right side

      i++; // Increment i to skip the new right side of the double page
    }
  }
}

void decode_jpeg(uint8_t *input_buffer, size_t input_size,
                 struct jpeg_decompress_struct *cinfo, JSAMPARRAY *pixel_data) {
  struct jpeg_error_mgr jerr;

  // Setup error handling
  cinfo->err = jpeg_std_error(&jerr);

  // Initialize JPEG decompression object and specify data source
  jpeg_create_decompress(cinfo);
  jpeg_mem_src(cinfo, input_buffer, input_size);

  // Read header and start decompression
  jpeg_read_header(cinfo, TRUE);
  jpeg_start_decompress(cinfo);

  // Allocate memory for one scanline
  *pixel_data = (*cinfo->mem->alloc_sarray)(
      (j_common_ptr)cinfo, JPOOL_IMAGE,
      cinfo->output_width * cinfo->output_components, 1);

  // Read scanlines
  while (cinfo->output_scanline < cinfo->output_height) {
    jpeg_read_scanlines(cinfo, *pixel_data + cinfo->output_scanline,
                        cinfo->output_height - cinfo->output_scanline);
  }
}

void write_half_jpeg(struct jpeg_decompress_struct *cinfo,
                     JSAMPARRAY pixel_data, int start_col, int width,
                     const char *filename) {
  struct jpeg_compress_struct cinfo_out;
  struct jpeg_error_mgr       jerr_out;
  FILE                       *outfile;
  JSAMPARRAY row_pointer = (JSAMPARRAY)malloc(sizeof(JSAMPROW));

  cinfo_out.err = jpeg_std_error(&jerr_out);
  jpeg_create_compress(&cinfo_out);

  if ((outfile = fopen(filename, "wb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    exit(1);
  }

  jpeg_stdio_dest(&cinfo_out, outfile);

  cinfo_out.image_width      = width;
  cinfo_out.image_height     = cinfo->output_height;
  cinfo_out.input_components = cinfo->output_components;
  cinfo_out.in_color_space   = cinfo->out_color_space;

  jpeg_set_defaults(&cinfo_out);
  jpeg_start_compress(&cinfo_out, TRUE);

  while (cinfo_out.next_scanline < cinfo_out.image_height) {
    row_pointer[0] = &pixel_data[cinfo_out.next_scanline]
                                [start_col * cinfo->output_components];
    jpeg_write_scanlines(&cinfo_out, row_pointer, 1);
  }

  jpeg_finish_compress(&cinfo_out);
  fclose(outfile);
  jpeg_destroy_compress(&cinfo_out);
  free(row_pointer);
}

void split_and_save(uint8_t *jpeg_buffer, size_t jpeg_size) {
  struct jpeg_decompress_struct cinfo;
  JSAMPARRAY                    pixel_data;

  // Decode JPEG to raw image
  decode_jpeg(jpeg_buffer, jpeg_size, &cinfo, &pixel_data);

  // Assuming the image is 2000x500, resize to 1000x500 and split
  int new_width = cinfo.output_width / 2;

  // Write left half
  write_half_jpeg(&cinfo, pixel_data, 0, new_width, "left.jpeg");

  // Write right half
  write_half_jpeg(&cinfo, pixel_data, new_width, new_width, "right.jpeg");

  // Cleanup
  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  free(pixel_data);
}

void make_output_cbz(const cli_flags_t *cli_flags, photo_t *photos,
                     uint32_t photo_count, const char *output_file) {
  int    err;
  zip_t *dest_zip = zip_open(output_file, ZIP_CREATE | ZIP_TRUNCATE, &err);

  if (!dest_zip) {
    fprintf(stderr, "Failed to open destination zip file: %s\n",
            zip_strerror(dest_zip));
    return;
  }

  for (uint32_t i = 0; i < photo_count; i++) {
    // Open the source zip archive
    int    src_err;
    zip_t *src_zip = zip_open(photos[i].cbz_path, 0, &src_err);
    if (!src_zip) {
      fprintf(stderr, "Failed to open source zip file: %s\n",
              zip_strerror(src_zip));
      continue;
    }

    // Find the index of the PNG file within the source zip
    zip_int64_t idx = zip_name_locate(src_zip, photos[i].name, 0);
    if (idx < 0) {
      fprintf(stderr, "File %s not found in source zip archive\n",
              photos[i].name);
      zip_close(src_zip);
      continue;
    }

    // Extract the PNG file from the source zip
    zip_file_t *zfile = zip_fopen_index(
        src_zip, idx,
        0); // this is the raw data for the png, this will be stored in a buffer
    if (!zfile) {
      fprintf(stderr, "Error opening file %s within source zip archive\n",
              photos[i].name);
      zip_close(src_zip);
      continue;
    }

    zip_stat_t zstat;
    zip_stat_index(src_zip, idx, 0, &zstat);
    uint8_t *buffer = malloc(zstat.size);
    if (buffer == NULL) {
      fprintf(stderr, "Memory allocation error\n");
      zip_fclose(zfile);
      zip_close(src_zip);
      continue;
    }

    zip_int64_t bytes_read = zip_fread(zfile, buffer, zstat.size);
    if (bytes_read < 0) {
      fprintf(stderr, "Error reading file %s within source zip archive\n",
              photos[i].name);
      free(buffer);
      zip_fclose(zfile);
      zip_close(src_zip);
      continue;
    }

    if (photos[i].ext[0] == '\0') {
      fprintf(stderr, "Error file has unkown type : %s\n", photos[i].name);
    }

    // Create a new source from buffer to add to the destination zip
    char new_filename[256]; // Ensure this buffer is large enough for the
                            // resulting filename
    snprintf(new_filename, sizeof(new_filename), "%d%s", photos[i].id,
             photos[i].ext);
    zip_source_t *source = zip_source_buffer(dest_zip, buffer, zstat.size, 1);
    if (!source) {
      fprintf(stderr, "Error creating zip source from buffer\n");
      free(buffer);
      zip_fclose(zfile);
      zip_close(src_zip);
      continue;
    }

    // DOUBLE PAGE LOGIC
    if (photos[i].double_page == DOUBLE_PAGE_FALSE) {
      continue; // XXX REMOVE ME
      // Add the image file to the destination zip archive
      if (zip_file_add(dest_zip, new_filename, source, ZIP_FL_OVERWRITE) < 0) {
        fprintf(stderr, "Error adding file %s to destination zip archive: %s\n",
                photos[i].name, zip_strerror(dest_zip));
        zip_source_free(source);
        zip_fclose(zfile);
        zip_close(src_zip);
        continue;
      }

      // Clean up
      zip_fclose(zfile);
      zip_close(src_zip);
      continue;
    }

    // The RAW data is in `buffer`

    /// save left and right
    split_and_save(buffer, zstat.size);

    if (photos[i].double_page == DOUBLE_PAGE_FIRST) {
      if (strcmp(photos[i].ext, ".png") == 0) {
        // TODO - do this later
      } else if (strcmp(photos[i].ext, ".jpg") == 0) {
      } else {
        fprintf(stderr, "Error: Unkonwn file extension %s\n", photos[i].ext);
      }
      continue;
    }

    if (photos[i].double_page == DOUBLE_PAGE_SECOND) {
      if (strcmp(photos[i].ext, ".png") == 0) {
        // TODO - do this later
      } else if (strcmp(photos[i].ext, ".jpg") == 0) {
        // XXX: TODO
      } else {
        fprintf(stderr, "Error: Unkonwn file extension %s\n", photos[i].ext);
      }
    } else {
      fprintf(stderr, "Error: Page is double, but not first nor second: %s\n",
              photos[i].name);
    }
  }

  if (zip_close(dest_zip) < 0) {
    fprintf(stderr, "Failed to close destination zip archive: %s\n",
            zip_strerror(dest_zip));
  }
}

void extract_and_combine_cbz(const cli_flags_t   *cli_flags,
                             const file_entry_t **sorted_files,
                             const char          *output_file,
                             const uint32_t      *file_count) {
  uint32_t photo_counter  = 0;
  uint32_t photos_arr_len = 10; // Initial array size
  photo_t *photos         = malloc(photos_arr_len * sizeof(photo_t));
  if (photos == NULL) {
    fprintf(stderr, "Failed to allocate memory for photos\n");
    return;
  }

  for (uint32_t i = 0; i < *file_count; i++) {
    _handle_cbz_entry(cli_flags, sorted_files[i]->filename, &photo_counter,
                      &photos, &photos_arr_len);
  }
  adjust_for_double_pages(&photos, &photo_counter);
  for (uint32_t i = 0; i < photo_counter; i++) {
    printfv(*cli_flags, "",
            "cbz_path: %s | id: %u | name: %s | width: %u | height: %u | "
            "double_page: %u | on_its_side: %u\n",
            photos[i].cbz_path, photos[i].id, photos[i].name, photos[i].width,
            photos[i].height, photos[i].double_page, photos[i].on_its_side);
  }
  make_output_cbz(cli_flags, photos, photo_counter, output_file);

  for (uint32_t i = 0; i < photo_counter; i++) {
    free(photos[i].cbz_path);
    free(photos[i].name);
  }

  free(photos);
}
