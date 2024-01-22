#include "extract.h"
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
int32_t extract_file_name_number(const char           *filename,
                                 const verbose_mode_e *verbose_mode,
                                 const color_mode_e   *color_mode) {
  regex_t     regex;
  regmatch_t  group_array[2];
  const char *pattern = "\\[(.*?)\\]";
  printfv(*verbose_mode, *color_mode, DARK_ORANGE, "Trying to find %s in %s\n",
          pattern, filename);
  regcomp(&regex, pattern, REG_EXTENDED);

  if (regexec(&regex, filename, 2, group_array, 0) != 0) {
    printfv(*verbose_mode, *color_mode, RED, "Regex failed\n");
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

file_type_e is_photo(const char *filename) {
  char *ext = strrchr(filename, '.');
  if (!ext || ext == filename) {
    return FILE_TYPE_E_OTHER; // 0/false
  }

  if (strcasecmp(ext, ".png") == 0) {
    return PNG;
  }

  if (strcasecmp(ext, ".jpeg") == 0 || strcasecmp(ext, ".jpg") == 0) {
    return JPEG;
  }

  return FILE_TYPE_E_OTHER;
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

void extract_and_combine_cbz(const file_entry_t  **sorted_files,
                             const char           *output_file,
                             const uint32_t       *file_count,
                             const verbose_mode_e *verbose_mode,
                             const color_mode_e   *color_mode) {
  int         err  = 0;
  struct zip *dest = zip_open(output_file, ZIP_CREATE | ZIP_TRUNCATE, &err);
  if (dest == NULL) {
    printfv(*verbose_mode, *color_mode, RED, "Failed to open <%s>\n",
            output_file);
    return;
  }

  uint32_t total_photos = 0;
  for (uint32_t i = 0; i < *file_count; ++i) {
    struct zip *z = zip_open((*sorted_files)[i].filename, 0, &err);
    if (z == NULL) {
      printfv(*verbose_mode, *color_mode, RED, "Failed to open <%s>\n",
              (*sorted_files)[i].filename);
      continue;
    }

    zip_uint64_t num_entries = zip_get_num_entries(z, 0);
    for (zip_uint64_t j = 0; j < num_entries; ++j) {
      const char *name = zip_get_name(z, j, 0);
      if (is_photo(name)) {
        total_photos++;
      }
    }

    zip_close(z);
  }

  int      padding_length = snprintf(NULL, 0, "%u", total_photos);
  uint32_t photo_counter  = 0;
  for (uint32_t i = 0; i < *file_count; ++i) {
    struct zip *z = zip_open((*sorted_files)[i].filename, 0, &err);
    if (z == NULL) {
      printfv(*verbose_mode, *color_mode, RED, "Failed to open <%s>\n",
              (*sorted_files)[i].filename);
      continue;
    }

    zip_uint64_t num_entries = zip_get_num_entries(z, 0);
    for (zip_uint64_t j = 0; j < num_entries; ++j) {
      const char *name     = zip_get_name(z, j, 0);
      const char *ext      = strchr(name, '.');
      file_type_e filetype = is_photo(name);
      if (is_dir(name) || filetype == FILE_TYPE_E_OTHER) {
        printfv(*verbose_mode, *color_mode, RED,
                "This is not a supported photo type <%s>\n",
                (*sorted_files)[i].filename);
        continue;
      }

      struct zip_file *file = zip_fopen(z, name, 0);
      struct zip_stat  st;
      zip_stat_init(&st);
      zip_stat(z, name, 0, &st);

      char *contents =
          (char *)mallocv("contents", *verbose_mode, *color_mode, st.size, -1);
      zip_fread(file, contents, st.size);

      printfv(*verbose_mode, *color_mode, "", "Handling file <%s>\n", name);

      uint32_t width = 0, height = 0;

      /// Note: cannot use filetype variable since it might be named as a png
      /// but be a jpg
      if (is_png((unsigned char *)contents, st.size)) {
        printfv(*verbose_mode, *color_mode, DARK_YELLOW,
                "File is detected as a PNG\n");
        get_png_dimensions_from_memory((unsigned char *)contents, st.size,
                                       &width, &height, verbose_mode,
                                       color_mode);
        ext = ".png";
      } else if (is_jpeg((unsigned char *)contents, st.size)) {
        printfv(*verbose_mode, *color_mode, DARK_YELLOW,
                "File is detected as a JPEG\n");
        get_jpeg_dimensions_from_memory((unsigned char *)contents, st.size,
                                        &width, &height, verbose_mode,
                                        color_mode);
        ext = ".jpg";
      } else {
        printfv(*verbose_mode, *color_mode, RED,
                "Unknown or unsupported file format <%s>\n", name);
        continue;
      }
      if (width == 0 || height == 0) {
        printfv(*verbose_mode, *color_mode, RED,
                "Error file has width or height of 0 <%s>\n", name);
        continue;
      }
      char new_filename[256];
      snprintf(new_filename, sizeof(new_filename), "%0*u%s", padding_length,
               photo_counter++, ext);

      printfv(
          *verbose_mode, *color_mode, "",
          "File <%s> which is now <%s> has width = <%u> and height = <%u>\n",
          name, new_filename, width, height);

      zip_source_t *src = zip_source_buffer(dest, contents, st.size, 1);
      zip_file_add(dest, new_filename, src, ZIP_FL_OVERWRITE);
      zip_fclose(file);
    }

    zip_close(z);
  }
  zip_close(dest);
}

void png_memory_read(png_structp png_ptr, png_bytep data, png_size_t length) {
  memory_reader_state_t *state =
      (memory_reader_state_t *)png_get_io_ptr(png_ptr);
  if (state->offset + length <= state->size) {
    memcpy(data, state->data + state->offset, length);
    state->offset += length;
  } else {
    png_error(png_ptr, "Read error in png_memory_read");
  }
}

void get_png_dimensions_from_memory(const unsigned char *data, size_t size,
                                    uint32_t *width, uint32_t *height,
                                    const verbose_mode_e *verbose_mode,
                                    const color_mode_e   *color_mode) {
  png_image image;
  memset(&image, 0, sizeof(image));
  image.version = PNG_IMAGE_VERSION;
  image.opaque  = NULL;

  if (!png_image_begin_read_from_memory(&image, data, size)) {
    printfv(*verbose_mode, *color_mode, RED, "Error processing PNG image\n");
    return;
  }
  *width  = image.width;
  *height = image.height;

  png_image_free(&image);
}

void get_jpeg_dimensions_from_memory(const unsigned char *data, size_t size,
                                     uint32_t *width, uint32_t *height,
                                     const verbose_mode_e *verbose_mode,
                                     const color_mode_e   *color_mode) {
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
        "row_pointer", *verbose_mode, *color_mode,
        (size_t)(cinfo.output_width * cinfo.output_components), -1);
    jpeg_read_scanlines(&cinfo, &row_pointer, 1);
    freev(*verbose_mode, *color_mode, row_pointer, "row_pointer", -1);
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
}
