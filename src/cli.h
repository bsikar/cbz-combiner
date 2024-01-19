#ifndef CLI_H
#define CLI_H

#include "extras.h"
#include <stdint.h>

typedef enum { INPUT_MODE_E_NONE, FILES, DIRECTORIES } input_mode_e;

/**
 * Frees memory allocated for input files
 *
 * @param input Pointer to array of input file names
 * @param input_count Pointer to the number of files
 * @param verbose_mode Pointer to the verbose mode flag
 * @param color_mode Pointer to the color mode flag
 * @return void
 */
void free_input(char **input, const uint32_t *input_count,
                const verbose_mode_e *verbose_mode,
                const color_mode_e   *color_mode);

/**
 * Frees memory allocated for output file
 *
 * @param output_file Pointer to the output file name
 * @param verbose_mode Pointer to the verbose mode flag
 * @param color_mode Pointer to the color mode flag
 * @return void
 */
void free_output_file(char **output_file, const verbose_mode_e *verbose_mode,
                      const color_mode_e *color_mode);

/**
 * Frees memory allocated for input files and output file
 *
 * @param input_files Pointer to array of input file names
 * @param file_count Pointer to the number of files
 * @param output_file Pointer to the output file name
 * @param verbose_mode Pointer to the verbose mode flag
 * @param color_mode Pointer to the color mode flag
 * @return void
 */
void free_memory(char **input_files, const uint32_t *file_count,
                 char **output_file, const verbose_mode_e *verbose_mode,
                 const color_mode_e *color_mode);

/**
 * Handles command line arguments, populates input files, output file name, and
 * flags
 *
 * @param argc Pointer to the argument count
 * @param argv Pointer to the argument vector
 * @param input_count Pointer to the file/dir count
 * @param output_file Pointer to the output file name
 * @param input Pointer to array of input file/dir names
 * @param verbose_mode Pointer to the verbose mode flag
 * @param input_mode Pointer to the input mode flag
 * @param color_mode Pointer to the color mode flag
 * @return void
 */
void handle_cli(const int *argc, const char **argv, uint32_t *input_count,
                char **output_file, char **input, verbose_mode_e *verbose_mode,
                input_mode_e *input_mode, color_mode_e *color_mode);

#endif // CLI_H
