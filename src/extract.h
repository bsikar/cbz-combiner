#ifndef EXTRACT_H
#define EXTRACT_H

#include "extras.h"
#include "file_entry_t.h"
#include <png.h>
#include <stdint.h>

typedef enum {
  FILE_TYPE_E_OTHER,
  PNG,
  JPEG,
} file_type_e;

typedef struct {
  unsigned char *data;
  size_t         size;
  size_t         offset;
} memory_reader_state_t;

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
 * @param filename The file name
 * @return file_type_e
 */
file_type_e is_photo(const char *filename);

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

// TODO: ADD DOC COMMENTS HERE
void png_memory_read(png_structp png_ptr, png_bytep data, png_size_t length);

void get_png_dimensions_from_memory(const unsigned char *data, size_t size,
                                    uint32_t *width, uint32_t *height,
                                    const verbose_mode_e *verbose_mode,
                                    const color_mode_e   *color_mode);

void get_jpeg_dimensions_from_memory(const unsigned char *data, size_t size,
                                    uint32_t *width, uint32_t *height,
                                    const verbose_mode_e *verbose_mode,
                                    const color_mode_e   *color_mode);
bool is_png(const unsigned char *data, size_t size);
bool is_jpeg(const unsigned char *data, size_t size);

#endif // EXTRACT_H
