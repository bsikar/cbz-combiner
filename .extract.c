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
bool _get_width_and_height(const cli_flags_t *cli_flags, uint32_t *width,
                           uint32_t *height, char **ext,
                           const unsigned char *contents, struct zip_stat *st) {
  // Note: cannot use filetype variable since it might be named as a png
  // but be a jpg
  if (is_png((unsigned char *)contents, st->size)) {
    printfv(*cli_flags, DARK_YELLOW, "File is detected as a PNG\n");
    _get_png_dimensions_from_memory(cli_flags, (unsigned char *)contents,
                                    st->size, width, height);
    *ext = ".png";
    return true;
  }
  if (is_jpeg((unsigned char *)contents, st->size)) {
    printfv(*cli_flags, DARK_YELLOW, "File is detected as a JPEG\n");
    _get_jpeg_dimensions_from_memory(cli_flags, (unsigned char *)contents,
                                     st->size, width, height);
    *ext = ".jpg";
    return true;
  }
  return false;
}

bool _handle_dimensions(const cli_flags_t *cli_flags, uint32_t *width,
                        uint32_t *height, const char *name,
                        unsigned char *contents, char **ext,
                        struct zip_stat *st) {
  if (!_get_width_and_height(cli_flags, width, height, ext, contents, st)) {
    printfv(*cli_flags, RED, "Unknown or unsupported file format <%s>\n", name);
    return false;
  }
  if (width == 0 || height == 0) {
    printfv(*cli_flags, RED, "Error file has width or height of 0 <%s>\n",
            name);
    return false;
  }
  return true;
}

void _handle_cbz_entry(const cli_flags_t *cli_flags, struct zip *z,
                       uint32_t *photo_counter, photo_t **photos,
                       uint32_t *photos_arr_len) {
  zip_uint64_t num_entries = zip_get_num_entries(z, 0);
  for (zip_uint64_t j = 0; j < num_entries; ++j) {
    if (j >= *photos_arr_len) {
      *photos_arr_len *= 2;
      *photos          = realloc(*photos, *photos_arr_len * sizeof(photo_t));
    }

    const char *name     = zip_get_name(z, j, 0);
    char       *ext      = strchr(name, '.');
    bool        filetype = is_photo(name);
    if (is_dir(name) || !filetype) {
      printfv(*cli_flags, RED, "This is not a supported photo type <%s>\n",
              name);
      continue;
    }

    struct zip_file *file = zip_fopen(z, name, 0);
    struct zip_stat  st;
    zip_stat_init(&st);
    zip_stat(z, name, 0, &st);

    unsigned char *contents =
        (unsigned char *)mallocv(*cli_flags, "contents", st.size, -1);
    zip_fread(file, contents, st.size);

    printfv(*cli_flags, "", "Handling file <%s>\n", name);

    uint32_t width = 0, height = 0;
    if (!_handle_dimensions(cli_flags, &width, &height, name, contents, &ext,
                            &st)) {
      continue; // erorrs printed in func
    }
    freev(*cli_flags, contents, "contents", -1);
    if (width == height) {
      printfv(*cli_flags, "", "Photo %s is square, skipping", name);
      continue;
    }

    // rename
    char new_filename[256];
    snprintf(new_filename, sizeof(new_filename), "%u%s", (*photo_counter)++,
             ext);

    printfv(*cli_flags, "",
            "File <%s> which is now <%s> has width = <%u> and height = <%u>\n",
            name, new_filename, width, height);

    // photo_t
    // name
    size_t len = strlen(new_filename) + 1;
    (*photos)[j].name =
        (char *)mallocv(*cli_flags, "(*photos)[].name", len, (uint32_t)j);
    strncpyv(*cli_flags, (*photos)[j].name, new_filename, len, len);

    (*photos)[j].width       = width;
    (*photos)[j].height      = height;
    (*photos)[j].id          = (*photo_counter) - 1;
    (*photos)[j].on_its_side = DOUBLE_PAGE_FALSE;
    (*photos)[j].double_page = DOUBLE_PAGE_FALSE;

    if (width > height) { // its on its side or its  a double page
      printfv(*cli_flags, "", "%s is on its side or a double page\n", name);
      (*photos)[j].on_its_side = DOUBLE_PAGE_TRUE;
      (*photos)[j].double_page = DOUBLE_PAGE_TRUE;
    }

    zip_fclose(file);
  }

  zip_close(z);
}

void _find_rotated_and_double_pages(const cli_flags_t *cli_flags,
                                    uint32_t *photo_counter, photo_t **photos) {
  uint32_t tmp_count = *photo_counter;
  for (uint32_t i = 0; i < tmp_count; ++i) {
    photo_t curr_photo = (*photos)[i];
    if (curr_photo.double_page && curr_photo.on_its_side) {
      if (curr_photo.width * 2 >= curr_photo.height) {
        curr_photo.on_its_side = DOUBLE_PAGE_FALSE;
        printfv(*cli_flags, "", "%s is a double page\n", curr_photo.name);
      } else {
        curr_photo.double_page = DOUBLE_PAGE_FALSE;
        printfv(*cli_flags, "", "%s is on its side\n", curr_photo.name);
      }
      (*photo_counter)++;
    }
  }
}
void _split_double_pages(const cli_flags_t *cli_flags,
                         const uint32_t    *photo_count_before,
                         const uint32_t *photo_counter, photo_t **photos) {
  /// We will need to do the following:
  /// - Iter until a double page is hit
  /// - Cut the double page into 2 regular sized pages (1/2 of the double page)
  /// - Save the new 2 pages as they should be:
  ///    - if 34.png is a double page
  ///    - then 34.png turns into:
  ///    - 34.png and 35.png
  /// - Calculate the shift needed for the rest of the following photos
  /// - At first the shift is 1 since there is only 1 double page => 1 extra
  ///   page
  /// - If there are more double pages then the shift will increase by 1
  ///
  /// - Rename all following files after the double page(s) to reflect the
  ///   proper name, that is now +shift away from their original value

  uint32_t shift = 0;
  // 1. iter until double page is hit
  for (uint32_t i = 0; i < *photo_count_before; ++i) {
    if ((*photos)[i].double_page == DOUBLE_PAGE_FALSE) {
      continue;
    }
    // Hit!
    photo_t *curr_photo      = &(*photos)[i];
    photo_t *new_photo       = &(*photos)[*photo_count_before + shift];
    (*photos)[i].double_page = DOUBLE_PAGE_FIRST;

    new_photo->id          = curr_photo->id;
    new_photo->width       = curr_photo->width;
    new_photo->height      = curr_photo->height;
    new_photo->on_its_side = DOUBLE_PAGE_FALSE;
    new_photo->double_page = DOUBLE_PAGE_SECOND;
    // cbz_path
    size_t len             = strlen(curr_photo->cbz_path) + 1;
    new_photo->cbz_path =
        (char *)mallocv(*cli_flags, "new_photo->cbz_path", len, -1);
    strncpyv(*cli_flags, new_photo->cbz_path, curr_photo->cbz_path, len, len);
    // name
    len             = strlen(curr_photo->name) + 1;
    new_photo->name = (char *)mallocv(*cli_flags, "new_photo->name", len, -1);
    strncpyv(*cli_flags, new_photo->name, curr_photo->name, len, len);

    shift += 1;
  }
}

void extract_and_combine_cbz(const cli_flags_t   *cli_flags,
                             const file_entry_t **sorted_files,
                             const char          *output_file,
                             const uint32_t      *file_count) {
  uint32_t photos_arr_len = (*file_count * 30); // assume each cbz has 30 photos
  photo_t *photos         = (photo_t *)mallocv(*cli_flags, "photos",
                                               photos_arr_len * sizeof(photo_t), -1);

  int      err;
  uint32_t photo_counter = 0, photo_count_before = 0;
  for (uint32_t i = 0; i < *file_count; ++i) {
    struct zip *z = zip_open((*sorted_files)[i].filename, 0, &err);
    if (z == NULL) {
      printfv(*cli_flags, RED, "Failed to open <%s>\n",
              (*sorted_files)[i].filename);
      continue;
    }

    size_t len = strlen((*sorted_files)[i].filename) + 1;
    photos[i].cbz_path =
        (char *)mallocv(*cli_flags, "photos[].cbz_path", len, i);
    strncpyv(*cli_flags, photos[i].cbz_path, (*sorted_files)[i].filename, len,
             len);

    _handle_cbz_entry(cli_flags, z, &photo_counter, &photos, &photos_arr_len);
    photo_count_before = photo_counter;
    _find_rotated_and_double_pages(cli_flags, &photo_counter, &photos);
    // check if we need to account for new photos
    if (photo_counter > photo_count_before) {
      photos_arr_len = photo_counter;
      photos         = realloc(photos, photos_arr_len * sizeof(photo_t));
    }
    // the new photos will be added to the end of the list for preformance
    //_split_double_pages(cli_flags, &photo_count_before, &photo_counter,
    //                    &photos);
  }

  for (uint32_t i = 0; i < photos_arr_len; ++i) {
    printf("%u %s\n", i, photos[i].cbz_path);
    //freev(*cli_flags, photos[i].name, "photos[].name", i);
    //freev(*cli_flags, photos[i].cbz_path, "photos[].cbz_path", i);
  }
  freev(*cli_flags, photos, "photos", -1);

  // update the dest zip
  struct zip *dest = zip_open(output_file, ZIP_CREATE | ZIP_TRUNCATE, &err);
  if (dest == NULL) {
    printfv(*cli_flags, RED, "Failed to open <%s>\n", output_file);
    return;
  }
  zip_close(dest);
}
