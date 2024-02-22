#ifndef EXTRACT_H
#define EXTRACT_H

#include "cli.h"
#include "file_entry_t.h"
#include <png.h>
#include <stdint.h>

typedef struct {
  unsigned char *data;
  size_t         size;
  size_t         offset;
} memory_reader_state_t;

/**
 * Extracts the number in a file's name
 *
 * @param cli_flags Pointer to the cli flags
 * @param filename Pointer to the file name
 * @return int32_t the number in the name
 */
int32_t extract_file_name_number(const cli_flags_t *cli_flags,
                                 const char        *filename);

/**
 * Extracts and combines cbz files
 *
 * @param cli_flags Pointer to the cli flags
 * @param softed_files Pointer to array of softed_files file names
 * @param file_count Pointer to the number of files
 * @return void
 */
void extract_and_combine_cbz(const cli_flags_t   *cli_flags,
                             const file_entry_t **softed_files,
                             const char          *output_file,
                             const uint32_t      *file_count);

/**
 * Checks if a file is a photo (png or jpeg)
 *
 * @param filename The file name
 * @return bool
 */
bool is_photo(const char *filename);

/**
 * Extracts the number in a file's name
 *
 * @param cli_flags Pointer to the cli flags
 * @param filename Pointer to the file name
 * @return int32_t the number in the name
 */
int32_t extract_file_name_number(const cli_flags_t *cli_flags,
                                 const char        *filename);

/**
 * Determines if a file is a png
 *
 * @param data Pointer of data associated with the file
 * @param size The size of the pointer
 * @return bool true if the file is a png
 */
bool is_png(const unsigned char *data, size_t size);

/**
 * Determines if a file is a jpeg
 *
 * @param data Pointer of data associated with the file
 * @param size The size of the pointer
 * @return bool true if the file is a jpeg
 */
bool is_jpeg(const unsigned char *data, size_t size);

#endif // EXTRACT_H
