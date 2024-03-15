/**
 * The goal of this program is to combine multiple .cbz files into one file
 *
 * A .cbz is common for manga and comics. It is the same as a .zip file with
 * multiple .pngs or other images inside.
 *
 * This program will extract all of the .zips (.cbzs) then compress them into
 * one larger .cbz or .pdf file.
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
    /* 0 */ "", // no error
    /* 1 */ "No input files were supplied",
    /* 2 */ "-o was used, but no output file was supplied",
    /* 3 */ "malloc or other memory error",
    /* 4 */ "--files and --dirs were both used",
    /* 5 */ "Invalid parameter passed",
    /* 6 */ "--files or --dirs were used but no files were supplied",
    /* 7 */ "Invalid file was supplied",
    /* 8 */ "Invlaid Directory was supplied"};

static void print_log_info(const cli_flags_t *cli_flags,
                           const char *output_file, const uint32_t *input_count,
                           const char **input);

static void handle_input_parsing(const cli_flags_t *cli_flags,
                                 uint32_t *input_count, uint32_t *file_count,
                                 char **input, file_entry_t **sorted_files);

int main(int argc, char **argv) {
  uint32_t    input_count = 0;
  cli_flags_t cli_flags   = {.input_mode   = INPUT_MODE_E_NONE,
                             .color_mode   = COLOR_DISABLED,
                             .verbose_mode = VERBOSE_MODE_E_NONE};
  char       *output_file = (char *)mallocv(cli_flags, "output_file",
                                            strlen(DEFAULT_OUTPUT_FILE_NAME) + 1, -1);
  strncpyv(cli_flags, output_file, DEFAULT_OUTPUT_FILE_NAME,
           strlen(DEFAULT_OUTPUT_FILE_NAME) + 1,
           strlen(DEFAULT_OUTPUT_FILE_NAME) + 1);
  char **input =
      (char **)mallocv(cli_flags, "input", argc * sizeof(char *), -1);
  file_entry_t *sorted_files = NULL;
  uint32_t      file_count   = 0;

  if (input == NULL) {
    print_error(3);
  }

  print_log_info(&cli_flags, output_file, &input_count, (const char **)input);

  handle_cli(&cli_flags, &argc, (const char **)argv, &input_count, &output_file,
             input);

  handle_input_parsing(&cli_flags, &input_count, &file_count, input,
                       &sorted_files);

  extract_and_combine_cbz(&cli_flags, (const file_entry_t **)&sorted_files,
                          output_file, &file_count);

  free_sorted_files(&cli_flags, &sorted_files, &file_count);
  free_output_file(&cli_flags, &output_file);

  return 0;
}

static void print_log_info(const cli_flags_t *cli_flags,
                           const char *output_file, const uint32_t *input_count,
                           const char **input) {
  if (cli_flags->verbose_mode) {
    printfv(*cli_flags, "", "verbose_mode: %d\n", cli_flags->verbose_mode);
    printfv(*cli_flags, "", "color_mode: %d\n", cli_flags->color_mode);
    printfv(*cli_flags, "", "input_mode: %d\n", cli_flags->input_mode);
    printfv(*cli_flags, "", "output_file: %s\n", output_file);
    printfv(*cli_flags, "", "output_file: %u\n", *input_count);
    for (uint32_t i = 0; i < *input_count; ++i) {
      printfv(*cli_flags, "", "\tFile %u : %s\n", i, input[i]);
    }
  }
}

static void handle_input_parsing(const cli_flags_t *cli_flags,
                                 uint32_t *input_count, uint32_t *file_count,
                                 char **input, file_entry_t **sorted_files) {
  // files is default
  if (cli_flags->input_mode == FILES ||
      cli_flags->input_mode == INPUT_MODE_E_NONE) {
    *sorted_files = (file_entry_t *)mallocv(
        *cli_flags, "sorted_files", *input_count * sizeof(file_entry_t), -1);
    // parse files and sort them
    printfv(*cli_flags, "", "Handling parsing files\n");
    handle_file_input_parsing(cli_flags, sorted_files, input, input_count);
    *file_count = *input_count;
  } else {
    *sorted_files = NULL;
    *file_count   = 0;
    printfv(*cli_flags, "", "Handling parsing dirs\n");
    handle_dir_input_parsing(cli_flags, sorted_files, input, input_count,
                             file_count);
  }
}
