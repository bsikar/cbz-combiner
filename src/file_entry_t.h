#ifndef FILE_ENTRY_T_H
#define FILE_ENTRY_T_H

#include "cli.h"
#include "extras.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint32_t number;
  char    *filename;
} file_entry_t;

typedef struct {
  char              *cbz_path;
  char              *name;
  char              *ext;
  uint32_t           width;
  uint32_t           height;
  uint32_t           id;
  bool               on_its_side;
  double_page_mode_e double_page;
} photo_t;

typedef struct {
  photo_t *photo1;
  photo_t *photo2;
} pdf_photo_t;

/**
 * Compares two file_entry_t instances
 *
 * @param a Pointer to the first file instance
 * @param b Pointer to the second file instance
 * @return int32_t the difference between the files
 */
int32_t compare_file_entry_ts(const void *a, const void *b);

/**
 * Adds and sorts files to the sorted_files list. It will also free the
 * input_files so no need to handle freeing outside of calling this function
 *
 * @param cli_flags Pointer to the cli flags
 * @param sorted_files Pointer to the sorted files list
 * @param input_files Pointer to the input files
 * @param file_count Pointer to the file count
 * @return void
 */
void handle_file_input_parsing(const cli_flags_t *cli_flags,
                               file_entry_t **sorted_files, char **input_files,
                               uint32_t *file_count);
/**
 * Adds and sorts files to the sorted_files list. It will also free the
 * input_dirs so no need to handle freeing outside of calling this function.
 * If there are duplicate files the dir passed in first will take precedence
 *
 * @param cli_flags Pointer to the cli flags
 * @param sorted_files Pointer to the sorted files list
 * @param input_dirs Pointer to the input files
 * @param dir_count Pointer to the dir count
 * @param file_count Pointer to the file count
 * @return void
 */
void handle_dir_input_parsing(const cli_flags_t *cli_flags,
                              file_entry_t **sorted_files, char **input_dirs,
                              const uint32_t *dir_count, uint32_t *file_count);

/**
 * Verifies that a string is a file
 *
 * @param path The file path
 * @return bool
 */
bool is_file(const char *path);

/**
 * Verifies that a string is a direectory
 *
 * @param path The dir path
 * @return bool
 */
bool is_dir(const char *path);

/**
 * Frees the sorted files
 *
 * @param cli_flags Pointer to the cli flags
 * @param sorted_files Pointer to the sorted files list
 * @param file_count Pointer to the file count
 * @return void
 */
void free_sorted_files(const cli_flags_t *cli_flags,
                       file_entry_t **sorted_files, const uint32_t *file_count);

#endif // FILE_ENTRY_T_H
