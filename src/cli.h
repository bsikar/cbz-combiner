#ifndef CLI_H
#define CLI_H

#include "extras.h"
#include <stdint.h>

typedef enum { INPUT_MODE_E_NONE, FILES, DIRECTORIES } input_mode_e;

typedef struct {
  verbose_mode_e verbose_mode;
  color_mode_e   color_mode;
  input_mode_e   input_mode;
  rotate_mode_e  rotate_mode;
  flip_mode_e    flip_mode;
} cli_flags_t;

/**
 * Frees memory allocated for input files
 *
 * @param cli_flags Pointer to the cli flags
 * @param input Pointer to array of input file names
 * @param input_count Pointer to the number of files
 * @return void
 */
void free_input(const cli_flags_t *cli_flags, char **input,
                const uint32_t *input_count);
/**
 * Frees memory allocated for output file
 *
 * @param cli_flags Pointer to the cli flags
 * @param output_file Pointer to the output file name
 * @return void
 */
void free_output_file(const cli_flags_t *cli_flags, char **output_file);
/**
 * Frees memory allocated for input files and output file
 *
 * @param cli_flags Pointer to the cli flags
 * @param input_files Pointer to array of input file names
 * @param file_count Pointer to the number of files
 * @param output_file Pointer to the output file name
 * @return void
 */
void free_memory(const cli_flags_t *cli_flags, char **input_files,
                 const uint32_t *file_count, char **output_file);
/**
 * Handles command line arguments, populates input files, output file name, and
 * flags
 *
 * @param cli_flags Pointer to the cli flags
 * @param argc Pointer to the argument count
 * @param argv Pointer to the argument vector
 * @param input_count Pointer to the file/dir count
 * @param output_file Pointer to the output file name
 * @param input Pointer to array of input file/dir names
 * @return void
 */
void handle_cli(cli_flags_t *cli_flags, const int *argc, const char **argv,
                uint32_t *input_count, char **output_file, char **input);

#endif // CLI_H
