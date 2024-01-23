#include "file_entry_t.h"
#include "cli.h"
#include "extract.h"
#include "extras.h"
#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int32_t compare_file_entry_ts(const void *a, const void *b) {
  file_entry_t *file_a = (file_entry_t *)a;
  file_entry_t *file_b = (file_entry_t *)b;
  return (file_a->number - file_b->number);
}

bool _process_file(const char *file, bool **number_map,
                   uint32_t *number_map_size, file_entry_t **sorted_files,
                   uint32_t *file_count, const verbose_mode_e *verbose_mode,
                   const color_mode_e *color_mode) {
  if (!is_file(file)) {
    printfv(*verbose_mode, *color_mode, RED, "Invalid file <%s>\n", file);
    return false;
  }

  int32_t number = extract_file_name_number(file, verbose_mode, color_mode);
  if (number == -1) {
    printfv(*verbose_mode, *color_mode, RED, "Failed to parse number <%s>\n",
            file);
    return false;
  }

  while (number > *number_map_size) {
    *number_map_size *= 2;
    *number_map       = reallocv(*verbose_mode, *color_mode, *number_map,
                                 "number_map", *number_map_size * sizeof(bool), -1);
  }

  if ((*number_map)[number]) { // if already exists
    return false;
  }

  (*number_map)[number] = true;
  printfv(*verbose_mode, *color_mode, DARK_GREEN,
          "Successful parsing: #%u (%i, %s)\n", *file_count, number, file);

  (*sorted_files)[*file_count].number = number;
  (*sorted_files)[*file_count].filename =
      (char *)mallocv("sorted_files[].filename", *verbose_mode, *color_mode,
                      strlen(file) + 1, *file_count);
  strcpy((*sorted_files)[*file_count].filename, file);
  (*file_count)++;

  return true;
}

void _sort_and_log_files(file_entry_t **sorted_files, uint32_t *file_count,
                         const verbose_mode_e *verbose_mode,
                         const color_mode_e   *color_mode) {
  *sorted_files = (file_entry_t *)reallocv(
      *verbose_mode, *color_mode, *sorted_files, "sorted_files",
      *file_count * sizeof(file_entry_t), -1);
  qsort(*sorted_files, *file_count, sizeof(file_entry_t),
        compare_file_entry_ts);

  if (*verbose_mode == VERY_VERBOSE) {
    printfv(*verbose_mode, *color_mode, "", "The parsed file data:\n");
    for (uint32_t i = 0; i < *file_count; ++i) {
      printfv(*verbose_mode, *color_mode, "", "  %p - %u - %s\n",
              &sorted_files[i], (*sorted_files)[i].number,
              (*sorted_files)[i].filename);
    }
  }
}

void handle_file_input_parsing(file_entry_t **sorted_files, char **input_files,
                               uint32_t             *file_count,
                               const verbose_mode_e *verbose_mode,
                               const color_mode_e   *color_mode) {
  uint32_t tmp_file_count  = 0;
  uint32_t number_map_size = INT16_MAX;
  bool    *number_map      = callocv("number_map", *verbose_mode, *color_mode,
                                     number_map_size, sizeof(bool), -1);
  for (uint32_t i = 0; i < *file_count; ++i) {
    if (_process_file(input_files[i], &number_map, &number_map_size,
                      sorted_files, &tmp_file_count, verbose_mode,
                      color_mode)) {
      freev(*verbose_mode, *color_mode, input_files[i], "input_files", i);
    }
  }

  // must free
  freev(*verbose_mode, *color_mode, number_map, "number_map", -1);
  free_input(input_files, file_count, verbose_mode, color_mode);

  *file_count = tmp_file_count;
  _sort_and_log_files(sorted_files, file_count, verbose_mode, color_mode);
}

char *_construct_file_path(const char *dir_name, const char *file_name,
                           const verbose_mode_e *verbose_mode,
                           const color_mode_e   *color_mode) {
  size_t dir_len            = strlen(dir_name);
  bool   has_trailing_slash = (dir_name[dir_len - 1] == '/');
  size_t file_length =
      dir_len + strlen(file_name) + (has_trailing_slash ? 1 : 2);

  char *file = mallocv("file", *verbose_mode, *color_mode, file_length, -1);
  if (!file)
    return NULL;

  if (has_trailing_slash) {
    snprintf(file, file_length, "%s%s", dir_name, file_name);
  } else {
    snprintf(file, file_length, "%s/%s", dir_name, file_name);
  }

  return file;
}

bool _process_directory(const char *dir_name, file_entry_t **sorted_files,
                        uint32_t *buffer_size, bool **number_map,
                        uint32_t *number_map_size, uint32_t *file_count,
                        const verbose_mode_e *verbose_mode,
                        const color_mode_e   *color_mode) {
  DIR           *d;
  struct dirent *dir;

  if (!is_dir(dir_name)) {
    printfv(*verbose_mode, *color_mode, RED, "Directory <%s> does not exist\n",
            dir_name);
    return false;
  }

  d = opendir(dir_name);
  if (d == NULL) {
    return false;
  }

  while ((dir = readdir(d)) != NULL) {
    char *file =
        _construct_file_path(dir_name, dir->d_name, verbose_mode, color_mode);
    if (!file || !_process_file(file, number_map, number_map_size, sorted_files,
                                file_count, verbose_mode, color_mode)) {
      freev(*verbose_mode, *color_mode, file, "file", -1);
      continue;
    }
    freev(*verbose_mode, *color_mode, file, "file", -1);

    // resize sorted_files array if needed
    if (*file_count == *buffer_size) {
      *buffer_size *= 2;
      *sorted_files =
          reallocv(*verbose_mode, *color_mode, *sorted_files, "sorted_files",
                   *buffer_size * sizeof(file_entry_t), -1);
    }
  }
  closedir(d);
  return true;
}

void handle_dir_input_parsing(file_entry_t **sorted_files, char **input_dirs,
                              const uint32_t *dir_count, uint32_t *file_count,
                              const verbose_mode_e *verbose_mode,
                              const color_mode_e   *color_mode) {
  uint32_t buffer_size = 50, number_map_size = INT16_MAX;
  *file_count      = 0;
  *sorted_files    = mallocv("sorted_files", *verbose_mode, *color_mode,
                             buffer_size * sizeof(file_entry_t), -1);
  bool *number_map = callocv("number_map", *verbose_mode, *color_mode,
                             number_map_size, sizeof(bool), -1);

  // process each directory
  for (uint32_t i = 0; i < *dir_count; ++i) {
    if (!_process_directory(input_dirs[i], sorted_files, &buffer_size,
                            &number_map, &number_map_size, file_count,
                            verbose_mode, color_mode)) {
      printfv(*verbose_mode, *color_mode, RED,
              "Directory processing failed for <%s>\n", input_dirs[i]);
    }
  }

  // must clean
  freev(*verbose_mode, *color_mode, number_map, "number_map", -1);
  free_input(input_dirs, dir_count, verbose_mode, color_mode);
  _sort_and_log_files(sorted_files, file_count, verbose_mode, color_mode);
}

bool is_file(const char *path) {
  struct stat statbuf;
  if (stat(path, &statbuf) != 0) {
    return false; // unable to get info, assume it's not a file
  }
  return S_ISREG(statbuf.st_mode);
}

bool is_dir(const char *path) {
  struct stat statbuf;
  if (stat(path, &statbuf) != 0) {
    return false; // unable to get info, assume it's not a directory
  }
  return S_ISDIR(statbuf.st_mode);
}

void free_sorted_files(file_entry_t **sorted_files, const uint32_t *file_count,
                       const verbose_mode_e *verbose_mode,
                       const color_mode_e   *color_mode) {
  if (sorted_files == NULL || file_count == NULL || verbose_mode == NULL) {
    printfv(*verbose_mode, *color_mode, RED,
            "Invalid null pointer argument(s) provided to free_sorted_files\n");
    return;
  }

  for (uint32_t i = 0; i < *file_count; ++i) {
    printfv(*verbose_mode, *color_mode, BLUE, "Freeing file entry: %s\n",
            (*sorted_files)[i].filename);
    freev(*verbose_mode, *color_mode, (*sorted_files)[i].filename,
          "sorted_files[].filename (index is)", i);
  }
  printfv(*verbose_mode, *color_mode, BLUE, "Freeing sorted_files: %p\n",
          *sorted_files);
  freev(*verbose_mode, *color_mode, *sorted_files, "sorted_files", -1);
}
