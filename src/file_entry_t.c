#include "file_entry_t.h"
#include "cli.h"
#include "extras.h"
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
    if (strstr(input_files[i], CBZ) == NULL || !is_file(input_files[i])) {
      continue;
    }
    int32_t number = extract_file_name_number((const char *)input_files[i],
                                              verbose_mode, color_mode);
    if (number == -1) {
      printfv(*verbose_mode, *color_mode, RED,
              "Failed to parse the number for %s\n", input_files[i]);
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
  qsort(sorted_files, *file_count, sizeof(file_entry_t), compare_file_entry_ts);

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

bool is_directory(const char *path) {
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
