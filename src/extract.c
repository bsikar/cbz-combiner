#define _POSIX_C_SOURCE 200809L
#include "extract.h"
#include "cli.h"
#include "extras.h"
#include "file_entry_t.h"
#include <hpdf.h>
#include <jpeglib.h>
#include <linux/limits.h> // for PATH_MAX
#include <math.h>
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
  // but be a jpeg
  if (is_png((unsigned char *)contents, st->size)) {
    printfv(*cli_flags, DARK_YELLOW, "File is detected as a PNG\n");
    _get_png_dimensions_from_memory(cli_flags, (unsigned char *)contents,
                                    st->size, width, height);
    strncpyv(*cli_flags, *ext, ".png\0", 5, 5);
    return true;
  }
  if (is_jpeg((unsigned char *)contents, st->size)) {
    printfv(*cli_flags, DARK_YELLOW, "File is detected as a JPEG\n");
    _get_jpeg_dimensions_from_memory(cli_flags, (unsigned char *)contents,
                                     st->size, width, height);
    strncpyv(*cli_flags, *ext, ".jpg\0", 5, 5);
    return true;
  }
  return false;
}

void _handle_cbz_entry(const cli_flags_t *cli_flags, const char *cbz_path,
                       uint32_t *photo_counter, photo_t **photos,
                       uint32_t *photos_arr_len) {
  // Open the zip file
  int    err;
  zip_t *dest = zip_open(cbz_path, 0, &err);
  if (!dest) {
    printfv(*cli_flags, RED, "Failed to open CBZ file: %s\n", cbz_path);
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

    unsigned char *contents =
        (unsigned char *)mallocv(*cli_flags, "contents", st.size, -1);
    if (!contents) {
      printfv(*cli_flags, RED, "Failed to allocate memory\n");
      zip_fclose(zf);
      continue;
    }

    zip_fread(zf, contents, st.size);

    // Use contents to get width and height
    uint32_t width, height;
    char    *ext = mallocv(*cli_flags, "ext", sizeof(char) * 5, -1);
    strncpyv(*cli_flags, ext, "\0\0\0\0\0", 5, 5);
    if (_get_width_height_and_type(cli_flags, &width, &height, &ext, contents,
                                   &st)) {
      // Check if we need to resize the photos array
      if (*photo_counter >= *photos_arr_len) {
        *photos_arr_len *= 2; // Double the size
        *photos          = reallocv(*cli_flags, *photos, "photos",
                                    *photos_arr_len * sizeof(photo_t), -1);
        if (*photos == NULL) {
          printfv(*cli_flags, RED, "Failed to reallocate memory\n");
          freev(*cli_flags, contents, "contents", -1);
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
      photo->double_page =
          (width > height) ? DOUBLE_PAGE_TRUE : DOUBLE_PAGE_FALSE;

      (*photo_counter)++;
    }

    freev(*cli_flags, contents, "contents", -1);
    freev(*cli_flags, ext, "ext", -1);
    zip_fclose(zf);
  }

  zip_close(dest);
}

bool _open_source_zip_archive(const cli_flags_t *cli_flags,
                              const char *cbz_path, zip_t **src_zip) {
  *src_zip = zip_open(cbz_path, 0, NULL);
  if (!(*src_zip)) {
    printfv(*cli_flags, RED, "Failed to open %s\n", cbz_path);
    return false;
  }
  return true;
}

bool _get_photo_index_from_source_zip(const cli_flags_t *cli_flags,
                                      zip_t *src_zip, const char *name,
                                      zip_int64_t *idx) {
  *idx = zip_name_locate(src_zip, name, 0);
  if (*idx < 0) {
    printfv(*cli_flags, RED, "Failed to find index for %s\n", name);
    return false;
  }
  return true;
}

/// XXX: FIXME: MEMORY LEAKS!!!
void _split_jpeg_buffer(const cli_flags_t *cli_flags, photo_t photo,
                        uint8_t **buffer, uint64_t *buff_size) {
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr         jerr;

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  // Setup decompression for buffer
  jpeg_mem_src(&cinfo, *buffer, *buff_size);
  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  int width      = cinfo.output_width;
  int height     = cinfo.output_height;
  int row_stride = width * cinfo.output_components;

  // Allocate memory for the raw image
  unsigned char *raw_image =
      (unsigned char *)malloc(width * height * cinfo.output_components);

  // Read scanlines
  while (cinfo.output_scanline < cinfo.output_height) {
    unsigned char *buffer_array[1];
    buffer_array[0] = raw_image + (cinfo.output_scanline) * row_stride;
    jpeg_read_scanlines(&cinfo, buffer_array, 1);
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  // Calculate new dimensions based on double_page flag
  int            new_width = width / 2;
  unsigned char *cropped_image =
      (unsigned char *)malloc(new_width * height * cinfo.output_components);

  // Crop the image
  for (int y = 0; y < height; y++) {
    memcpy(cropped_image + y * new_width * cinfo.output_components,
           raw_image + y * row_stride +
               (photo.double_page == DOUBLE_PAGE_RIGHT
                    ? new_width * cinfo.output_components
                    : 0),
           new_width * cinfo.output_components);
  }

  // Free original raw image
  free(raw_image);

  // Re-encode the cropped image back into JPEG
  struct jpeg_compress_struct cinfo_compress;
  struct jpeg_error_mgr       jerr_compress;
  unsigned long               new_buff_size = 0;
  unsigned char              *new_buffer    = NULL;

  cinfo_compress.err = jpeg_std_error(&jerr_compress);
  jpeg_create_compress(&cinfo_compress);
  jpeg_mem_dest(&cinfo_compress, &new_buffer, &new_buff_size);

  cinfo_compress.image_width      = new_width;
  cinfo_compress.image_height     = height;
  cinfo_compress.input_components = cinfo.output_components;
  cinfo_compress.in_color_space   = cinfo.out_color_space;

  jpeg_set_defaults(&cinfo_compress);
  jpeg_set_quality(&cinfo_compress, 100, TRUE);
  jpeg_start_compress(&cinfo_compress, TRUE);

  while (cinfo_compress.next_scanline < cinfo_compress.image_height) {
    unsigned char *row_pointer[1];
    row_pointer[0] = &cropped_image[cinfo_compress.next_scanline * new_width *
                                    cinfo.output_components];
    jpeg_write_scanlines(&cinfo_compress, row_pointer, 1);
  }

  jpeg_finish_compress(&cinfo_compress);
  jpeg_destroy_compress(&cinfo_compress);

  // Replace old buffer with new buffer
  freev(*cli_flags, *buffer, "buffer", -1);
  *buffer    = new_buffer;
  *buff_size = new_buff_size;

  // Free cropped image
  free(cropped_image);
}

void _pdf_error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no,
                        void *user_data) {
  printf("ERROR: error_no=%04X, detail_no=%u\n", (unsigned int)error_no,
         (unsigned int)detail_no);
  exit(-99);
}

bool _handle_buffer(const cli_flags_t *cli_flags, zip_t *src_zip,
                    zip_t **dest_zip, HPDF_Doc *pdf, const char *name,
                    zip_int64_t idx, photo_t photo) {
  /* extract png from source to buffer */
  zip_file_t *zfile = zip_fopen_index(src_zip, idx, 0);
  if (!zfile) {
    printfv(*cli_flags, RED,
            "Error opening file %s within source zip archive\n", name);
    return false;
  }

  zip_stat_t zstat;
  zip_stat_index(src_zip, idx, 0, &zstat);
  uint8_t *buffer = (uint8_t *)mallocv(*cli_flags, "buffer", zstat.size, -1);
  if (!buffer) {
    zip_fclose(zfile);
    return false;
  }

  zip_int64_t bytes_read = zip_fread(zfile, buffer, zstat.size);
  if (bytes_read < 0) {
    printfv(*cli_flags, RED, "Failed to read %s within source zip archive\n ",
            name);
    zip_fclose(zfile);
    freev(*cli_flags, buffer, "buffer", -1);
    return false;
  }
  zip_fclose(zfile);

  /* create newfilename */
  char new_filename[PATH_MAX];
  snprintf(new_filename, sizeof(new_filename), "%05u%s", photo.id, photo.ext);

  uint64_t buff_size = zstat.size;
  /* Handle double page */
  if (photo.double_page != DOUBLE_PAGE_FALSE) {
    // split the file [X|X] [X|_] or [_|X]
    // XXX : Currently working on this function
    if (strcmp(photo.ext, ".jpg") == 0) {
      _split_jpeg_buffer(cli_flags, photo, &buffer, &buff_size);
    } else {
      printfv(*cli_flags, RED, "Error: Unsupported file type\n");
      return false;
    }
  }

  /// ADD TO ZIP
  /* create new source from buffer */
  zip_source_t *zip_source = zip_source_buffer(*dest_zip, buffer, buff_size,
                                               1); // let libzip take ownership
  if (!zip_source) {
    // manually free buffer if zip_source creation fails
    freev(*cli_flags, buffer, "buffer", -1);
    return false;
  }

  /* add image to dest zip */
  if (zip_file_add(*dest_zip, new_filename, zip_source, ZIP_FL_OVERWRITE) < 0) {
    // this will also free the buffer since the zip_source owns it now
    zip_source_free(zip_source);
    return false;
  }

  /// ADD TO PDF
  HPDF_Page page = HPDF_AddPage(*pdf);
  HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);

  HPDF_Image image = HPDF_LoadJpegImageFromMem(*pdf, buffer, buff_size);
  if (!image) {
    return false;
  }
  // Get the dimensions of the image
  float img_width  = HPDF_Image_GetWidth(image);
  float img_height = HPDF_Image_GetHeight(image);

  // Get the dimensions of the page
  float page_width  = HPDF_Page_GetWidth(page);
  float page_height = HPDF_Page_GetHeight(page);

  // Calculate the x and y position to center the image
  float x = (page_width - img_width) / 2;
  float y = (page_height - img_height) / 2;

  // Check if the image is larger than the page size
  if (img_width > page_width || img_height > page_height) {
    // Scale the image down to fit the page
    float scale_ratio  = fmin(page_width / img_width, page_height / img_height);
    img_width         *= scale_ratio;
    img_height        *= scale_ratio;
    // Recalculate the x and y position for the scaled image
    x                  = (page_width - img_width) / 2;
    y                  = (page_height - img_height) / 2;
  }

  // Place the image on the page, centered
  HPDF_Page_DrawImage(page, image, x, y, img_width, img_height);

  // no need to free the buffer here, as it's now managed by libzip
  return true;
}

void _make_output_cbz(const cli_flags_t *cli_flags, photo_t **photos,
                      uint32_t *photo_count, const char *output_file) {
  zip_t   *dest_zip = zip_open(output_file, ZIP_CREATE | ZIP_TRUNCATE, NULL);
  HPDF_Doc pdf      = HPDF_New(_pdf_error_handler, NULL);
  if (!pdf) {
    return;
  }
  if (!dest_zip) {
    printfv(*cli_flags, RED, "Failed to open destination zip file: %s\n",
            zip_strerror(dest_zip));
    return;
  }

  for (uint32_t i = 0; i < *photo_count; i++) {
    if ((*photos)[i].ext[0] == '\0') {
      printfv(*cli_flags, RED, "File has unknown type: %s\n",
              (*photos)[i].name);
      continue;
    }
    zip_t *src_zip = NULL;
    if (!_open_source_zip_archive(cli_flags, (*photos)[i].cbz_path, &src_zip)) {
      continue;
    }

    zip_int64_t idx;
    if (!_get_photo_index_from_source_zip(cli_flags, src_zip, (*photos)[i].name,
                                          &idx)) {
      zip_close(src_zip);
      continue;
    }

    if (!_handle_buffer(cli_flags, src_zip, &dest_zip, &pdf, (*photos)[i].name,
                        idx, (*photos)[i])) {
      HPDF_Free(pdf);
      zip_close(src_zip);
      continue;
    }
    zip_close(src_zip);
  }
  zip_close(dest_zip);
  char *s = mallocv(*cli_flags, "s", strlen(output_file) + 5, -1);
  snprintf(s, strlen(output_file) + 5, "%s.pdf", output_file);
  HPDF_SaveToFile(pdf, s);
  freev(*cli_flags, s, "s", -1);
  HPDF_Free(pdf);
}

photo_t _deep_copy_photo(const photo_t *src) {
  photo_t copy;
  copy.cbz_path    = strdup(src->cbz_path);
  copy.name        = strdup(src->name);
  copy.ext         = strdup(src->ext);
  copy.width       = src->width;
  copy.height      = src->height;
  copy.id          = src->id;
  copy.on_its_side = src->on_its_side;
  copy.double_page = src->double_page;
  return copy;
}

void _reorder_double_page_photos(const cli_flags_t *cli_flags, photo_t **photos,
                                 uint32_t *photo_counter) {
  for (uint32_t i = 0; i < *photo_counter; i++) {
    if ((*photos)[i].double_page == DOUBLE_PAGE_FALSE)
      continue;
    if ((*photos)[i].double_page == DOUBLE_PAGE_TRUE) {
      *photos = realloc(*photos, sizeof(photo_t) * (*photo_counter + 1));
      for (uint32_t j = *photo_counter; j > i + 1; j--) {
        (*photos)[j] = (*photos)[j - 1]; // shift photos down by one
        (*photos)[j].id++;
      }
      (*photos)[i + 1]              = _deep_copy_photo(&(*photos)[i]);
      (*photos)[i + 1].id          += 1;
      (*photos)[i + 1].double_page  = DOUBLE_PAGE_RIGHT;
      (*photos)[i].double_page      = DOUBLE_PAGE_LEFT;
      (*photo_counter)++;
      i++; // skip the new photo to avoid reprocessing
    }
  }
}

void extract_and_combine_cbz(const cli_flags_t   *cli_flags,
                             const file_entry_t **sorted_files,
                             const char          *output_file,
                             const uint32_t      *file_count) {
  uint32_t photo_counter  = 0;
  uint32_t photos_arr_len = 10; // Initial array size
  photo_t *photos         = (photo_t *)mallocv(*cli_flags, "photos",
                                               photos_arr_len * sizeof(photo_t), -1);
  if (photos == NULL) {
    printfv(*cli_flags, RED, "Failed to allocate memory for photos\n");
    return;
  }

  for (uint32_t i = 0; i < *file_count; i++) {
    _handle_cbz_entry(cli_flags, (*sorted_files)[i].filename, &photo_counter,
                      &photos, &photos_arr_len);
  }
  // at this point we dont care about photos_arr_len
  // only photo_counter will be used
  photos = (photo_t *)reallocv(*cli_flags, photos, "photos",
                               sizeof(photo_t) * photo_counter, -1);
  // increase since and rename for double photos being left and right
  _reorder_double_page_photos(cli_flags, &photos, &photo_counter);
  for (uint32_t i = 0; i < photo_counter; i++) {
    printfv(*cli_flags, "",
            "cbz_path: %s | id: %u | name: %s | width: %u | height: %u | "
            "double_page: %u | on_its_side: %u\n",
            photos[i].cbz_path, photos[i].id, photos[i].name, photos[i].width,
            photos[i].height, photos[i].double_page, photos[i].on_its_side);
  }
  _make_output_cbz(cli_flags, &photos, &photo_counter, output_file);

  for (uint32_t i = 0; i < photo_counter; i++) {
    freev(*cli_flags, photos[i].cbz_path, "photos[].cbz_path", i);
    freev(*cli_flags, photos[i].name, "photos[].name", i);
    freev(*cli_flags, photos[i].ext, "photos[].ext", i);
  }

  freev(*cli_flags, photos, "photos", -1);
}
