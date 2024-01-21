#include "extract.h"
#include "extras.h"
#include "file_entry_t.h"
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

int is_photo(const char *filename) {
  char *dot = strrchr(filename, '.');
  if (!dot || dot == filename) {
    return 0;
  }
  return strcasecmp(dot, ".png") == 0 || strcasecmp(dot, ".jpeg") == 0;
}

void extract_and_combine_cbz(const file_entry_t  **sorted_files,
                             const char           *output_file,
                             const uint32_t       *file_count,
                             const verbose_mode_e *verbose_mode,
                             const color_mode_e   *color_mode) {
  int         err  = 0;
  struct zip *dest = zip_open(output_file, ZIP_CREATE | ZIP_TRUNCATE, &err);
  if (dest == NULL) {
    return;
  }

  uint32_t total_photos = 0;
  for (uint32_t i = 0; i < *file_count; ++i) {
    struct zip *z = zip_open((*sorted_files)[i].filename, 0, &err);
    if (z == NULL) {
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
      continue;
    }

    zip_uint64_t num_entries = zip_get_num_entries(z, 0);
    for (zip_uint64_t j = 0; j < num_entries; ++j) {
      const char *name = zip_get_name(z, j, 0);
      if (is_dir(name) || !is_photo(name)) {
        continue;
      }

      struct zip_file *file = zip_fopen(z, name, 0);
      struct zip_stat  st;
      zip_stat_init(&st);
      zip_stat(z, name, 0, &st);

      char *contents =
          (char *)mallocv("contents", *verbose_mode, *color_mode, st.size, -1);
      zip_fread(file, contents, st.size);

      char new_filename[256];
      snprintf(new_filename, sizeof(new_filename), "%0*u.png", padding_length,
               photo_counter++);
      printfv(*verbose_mode, *color_mode, "", "Handling file %s\n",
              new_filename);

      zip_source_t *src = zip_source_buffer(dest, contents, st.size, 1);
      zip_file_add(dest, new_filename, src, ZIP_FL_OVERWRITE);
      zip_fclose(file);
    }

    zip_close(z);
  }
  zip_close(dest);
}
