#ifndef FILE_ENTRY_T_H
#define FILE_ENTRY_T_H

#include "extras.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint32_t number;
  char    *filename;
} file_entry_t;

/**
 * Extracts the number in a file's name
 *
 * @param filename Pointer to the file name
 * @param verbose_mode Pointer to the verbose_mode flag
 * @param color_mode Pointer to the color mode flag
 * @return int32_t the number in the name
 */
int32_t extract_file_name_number(const char           *filename,
                                 const verbose_mode_e *verbose_mode,
                                 const color_mode_e   *color_mode);

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
 * @param sorted_files Pointer to the sorted files list
 * @param input_files Pointer to the input files
 * @param file_count Pointer to the file count
 * @param verbose_mode Pointer to the verbose_mode flag
 * @param color_mode Pointer to the color mode flag
 * @return void
 */
void handle_file_input_parsing(file_entry_t **sorted_files, char **input_files,
                               uint32_t             *file_count,
                               const verbose_mode_e *verbose_mode,
                               const color_mode_e   *color_mode);
/**
 * Adds and sorts files to the sorted_files list. It will also free the
 * input_dirs so no need to handle freeing outside of calling this function.
 * If there are duplicate files the dir passed in first will take precedence
 *
 * @param sorted_files Pointer to the sorted files list
 * @param input_dirs Pointer to the input files
 * @param dir_count Pointer to the dir count
 * @param file_count Pointer to the file count
 * @param verbose_mode Pointer to the verbose_mode flag
 * @param color_mode Pointer to the color mode flag
 * @return void
 */
void handle_dir_input_parsing(file_entry_t **sorted_files, char **input_dirs,
                              const uint32_t *dir_count, uint32_t *file_count,
                              const verbose_mode_e *verbose_mode,
                              const color_mode_e   *color_mode);

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
 * @param sorted_files Pointer to the sorted files list
 * @param file_count Pointer to the file count
 * @param verbose_mode Pointer to the verbose_mode flag
 * @param color_mode Pointer to the color mode flag
 * @return void
 */
void free_sorted_files(file_entry_t **sorted_files, const uint32_t *file_count,
                       const verbose_mode_e *verbose_mode,
                       const color_mode_e   *color_mode);

#endif // FILE_ENTRY_T_H
