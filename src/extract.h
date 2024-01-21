#ifndef EXTRACT_H
#define EXTRACT_H

#include "extras.h"
#include "file_entry_t.h"
#include <stdint.h>

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
 * Extracts and combines cbz files
 *
 * @param softed_files Pointer to array of softed_files file names
 * @param file_count Pointer to the number of files
 * @param verbose_mode Pointer to the verbose mode flag
 * @param color_mode Pointer to the color mode flag
 * @return void
 */
void extract_and_combine_cbz(const file_entry_t  **softed_files,
                             const char           *output_file,
                             const uint32_t       *file_count,
                             const verbose_mode_e *verbose_mode,
                             const color_mode_e   *color_mode);

/**
 * Checks if a file is a photo (png or jpeg)
 *
 * @param filename
 * @return int
 */
int is_photo(const char *filename);

#endif // EXTRACT_H
