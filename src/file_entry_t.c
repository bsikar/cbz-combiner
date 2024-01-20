#include "file_entry_t.h"
#include "cli.h"
#include "extras.h"
#include <dirent.h>
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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

int32_t compare_file_entry_ts(const void *a, const void *b) {
  file_entry_t *file_a = (file_entry_t *)a;
  file_entry_t *file_b = (file_entry_t *)b;
  return (file_a->number - file_b->number);
}

void handle_file_input_parsing(file_entry_t **sorted_files, char **input_files,
                               uint32_t             *file_count,
                               const verbose_mode_e *verbose_mode,
                               const color_mode_e   *color_mode) {
  uint32_t tmp_file_count = 0;
  for (uint32_t i = 0; i < *file_count; ++i) {
    if (input_files[i] == NULL || strstr(input_files[i], CBZ) == NULL ||
        !is_file(input_files[i])) {
      printfv(*verbose_mode, *color_mode, RED,
              "File either doesnt exist or is an invalid file type <%s>\n",
              input_files[i]);
    }
    int32_t number = extract_file_name_number((const char *)input_files[i],
                                              verbose_mode, color_mode);
    if (number == -1) {
      printfv(*verbose_mode, *color_mode, RED,
              "Failed to parse the number for <%s>\n", input_files[i]);
      continue;
    }
    printfv(*verbose_mode, *color_mode, DARK_GREEN,
            "Sucessful parsing: #%u (%i, %s)\n", i, number, input_files[i]);
    (*sorted_files)[tmp_file_count].number   = number;
    (*sorted_files)[tmp_file_count].filename = (char *)mallocv(
        "sorted_files[].filename (index is)", *verbose_mode, *color_mode,
        strlen(input_files[i]) + 1, tmp_file_count);
    strcpy((*sorted_files)[tmp_file_count++].filename, input_files[i]);
  }

  // must free
  free_input(input_files, file_count, verbose_mode, color_mode);
  *file_count   = tmp_file_count;
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

void handle_dir_input_parsing(file_entry_t **sorted_files, char **input_dirs,
                              const uint32_t *dir_count, uint32_t *file_count,
                              const verbose_mode_e *verbose_mode,
                              const color_mode_e   *color_mode) {
  DIR           *d;
  struct dirent *dir;
  uint32_t       buffer_size = 50;
  *sorted_files = (file_entry_t *)malloc(buffer_size * sizeof(file_entry_t));
  *file_count   = 0;
  bool *number_map =
      calloc(INT32_MAX, sizeof(bool)); // UUUHHH LOTS OF MEM like 1gb

  for (uint32_t i = 0; i < *dir_count; ++i) {
    if (!is_dir(input_dirs[i])) {
      printfv(*verbose_mode, *color_mode, RED,
              "Directory <%s> does not exist\n", input_dirs[i]);
      continue;
    }

    d = opendir(input_dirs[i]);
    if (d == NULL) {
      continue;
    }

    while ((dir = readdir(d)) != NULL) {
      size_t dir_len            = strlen(input_dirs[i]);
      bool   has_trailing_slash = (input_dirs[i][dir_len - 1] == '/');
      // +2 for '/' and '\0', +1 if it already has '/'
      size_t file_length =
          dir_len + strlen(dir->d_name) + (has_trailing_slash ? 1 : 2);
      char *file =
          (char *)mallocv("file", *verbose_mode, *color_mode, file_length, -1);
      if (has_trailing_slash) {
        snprintf(file, file_length, "%s%s", input_dirs[i], dir->d_name);
      } else {
        snprintf(file, file_length, "%s/%s", input_dirs[i], dir->d_name);
      }

      if (strstr(file, CBZ) == NULL || !is_file(file)) {
        printfv(*verbose_mode, *color_mode, RED,
                "File either doesnt exist or is an invalid file type <%s>\n",
                file);
        freev(*verbose_mode, *color_mode, file, "file", -1);
        continue;
      }

      int32_t number = extract_file_name_number((const char *)file,
                                                verbose_mode, color_mode);
      if (number == -1) {
        printfv(*verbose_mode, *color_mode, RED,
                "Failed to parse the number for <%s>\n", file);
        freev(*verbose_mode, *color_mode, file, "file", -1);
        continue;
      }

      if (number_map[number]) { // check if the number is already in the map
        freev(*verbose_mode, *color_mode, file, "file", -1);
        continue;
      }

      number_map[number]                  = true;
      (*sorted_files)[*file_count].number = number;
      (*sorted_files)[*file_count].filename =
          (char *)mallocv("sorted_files[].filename (index is)", *verbose_mode,
                          *color_mode, strlen(file) + 1, *file_count);
      strcpy((*sorted_files)[*file_count].filename, file);
      freev(*verbose_mode, *color_mode, file, "file", -1);
      (*file_count)++;

      if (*file_count == buffer_size) {
        buffer_size *= 2;
        *sorted_files =
            reallocv(*verbose_mode, *color_mode, *sorted_files, "sorted_files",
                     buffer_size * sizeof(file_entry_t), -1);
      }
    }
    closedir(d);
  }

  // must free
  free(number_map); // goodbye large 2GB array
  free_input(input_dirs, dir_count, verbose_mode, color_mode);
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
