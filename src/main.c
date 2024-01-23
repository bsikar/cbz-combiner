/**
 * The goal of this program is to combine multiple .cbz files into one file
 *
 * A .cbz is common for manga and comics. It is the same as a .zip file with
 * multiple .pngs or other images inside.
 *
 * This program will extract all of the .zips (.cbzs) then compress them into
 * one larger .cbz file.
 *
 * The only thing that will need to take account for is the naming of the pngs
 * inside the .cbz and their order (based off their names)
 */

#include "cli.h"
#include "extract.h"
#include "extras.h"
#include "file_entry_t.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const char *error_messages[] = {
    "", // no error
    "No input files were supplied",
    "-o was used, but no output file was supplied",
    "malloc or other memory error",
    "--files and --dirs were both used",
    "--files or --dirs were used but no files were supplied"};

void print_log_info(const verbose_mode_e *verbose_mode,
                    const color_mode_e   *color_mode,
                    const input_mode_e *input_mode, const char *output_file,
                    const uint32_t *input_count, const char **input);

void handle_input_parsing(const verbose_mode_e *verbose_mode,
                          const color_mode_e   *color_mode,
                          const input_mode_e   *input_mode,
                          const char *output_file, uint32_t *input_count,
                          uint32_t *file_count, char **input,
                          file_entry_t **sorted_files);

int main(int argc, char **argv) {
  uint32_t input_count = 0;
  char    *output_file = (char *)malloc(strlen(DEFAULT_OUTPUT_FILE_NAME) + 1);
  strcpy(output_file, DEFAULT_OUTPUT_FILE_NAME);
  verbose_mode_e verbose_mode = VERBOSE_MODE_E_NONE;
  color_mode_e   color_mode   = COLOR_DISABLED;
  char         **input = (char **)mallocv("input", verbose_mode, color_mode,
                                          argc * sizeof(char *), -1);
  input_mode_e   input_mode   = INPUT_MODE_E_NONE;
  file_entry_t  *sorted_files = NULL;
  uint32_t       file_count   = 0;

  if (input == NULL) {
    print_error(3);
  }

  print_log_info(&verbose_mode, &color_mode, &input_mode, output_file,
                 &input_count, (const char **)input);

  handle_cli(&argc, (const char **)argv, &input_count, &output_file, input,
             &verbose_mode, &input_mode, &color_mode);

  handle_input_parsing(&verbose_mode, &color_mode, &input_mode, output_file,
                       &input_count, &file_count, input, &sorted_files);

  extract_and_combine_cbz((const file_entry_t **)&sorted_files, output_file,
                          &file_count, &verbose_mode, &color_mode);

  free_sorted_files(&sorted_files, &file_count, &verbose_mode, &color_mode);
  free_output_file(&output_file, &verbose_mode, &color_mode);

  return 0;
}

void print_log_info(const verbose_mode_e *verbose_mode,
                    const color_mode_e   *color_mode,
                    const input_mode_e *input_mode, const char *output_file,
                    const uint32_t *input_count, const char **input) {
  if (*verbose_mode) {
    printfv(*verbose_mode, *color_mode, "", "%sVerbose Mode: ON\n",
            (*verbose_mode == VERY_VERBOSE) ? "Very " : "");
    printfv(*verbose_mode, *color_mode, "", "Color Mode: %s\n",
            *color_mode ? "ON" : "OFF");
    printfv(*verbose_mode, *color_mode, "", "Input %s:\n",
            (*input_mode == DIRECTORIES) ? "dirs" : "files");
    for (uint32_t i = 0; i < *input_count; ++i) {
      printfv(*verbose_mode, *color_mode, "", "  %s\n", input[i]);
    }
    printfv(*verbose_mode, *color_mode, "", "Output file: %s\n", output_file);
  }
}

void handle_input_parsing(const verbose_mode_e *verbose_mode,
                          const color_mode_e   *color_mode,
                          const input_mode_e   *input_mode,
                          const char *output_file, uint32_t *input_count,
                          uint32_t *file_count, char **input,
                          file_entry_t **sorted_files) {
  // files is default
  if (*input_mode == FILES || *input_mode == INPUT_MODE_E_NONE) {
    *sorted_files =
        (file_entry_t *)mallocv("sorted_files", *verbose_mode, *color_mode,
                                *input_count * sizeof(file_entry_t), -1);
    // parse files and sort them
    printfv(*verbose_mode, *color_mode, "", "Handling parsing files\n");
    handle_file_input_parsing(sorted_files, input, input_count, verbose_mode,
                              color_mode);
    *file_count = *input_count;
  } else {
    *sorted_files = NULL;
    *file_count   = 0;
    printfv(*verbose_mode, *color_mode, "", "Handling parsing dirs\n");
    handle_dir_input_parsing(sorted_files, input, input_count, file_count,
                             verbose_mode, color_mode);
  }
}
