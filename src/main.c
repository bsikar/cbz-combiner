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

/**
 * Day 1: some good progress -
 *        todo: there are many memory leaks. fix this.
 *              there is no progress on directories, do that
 *              unziping and combining still needs to be done
 *
 * Day 2: more progress + added -vv and -c (also fixed memory leaks)
 *        todo: there is no progress on directories, do that
 *              unziping and combining still needs to be done
 *
 * Day 3: added -d/--dirs
 *        todo: add the unzipping and combining
 *
 * Day 4: added unzipping and combining
 *        todo: could make it faster, but it works
 *
 * Day 5: found some errors that need to be fixed:
 *        - do more error handling
 *        - some files are not .png's, but are being renamed as a png
 *        - some photos are rly 2 photos next to each other
 *          -> some manga have panels stretch across and the images are 2 in 1
 *          -> so make [X|Y] into [X] and [Y]
 *          -> this can be done with libpng, but jpegs != png
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

  handle_cli(&argc, (const char **)argv, &input_count, &output_file, input,
             &verbose_mode, &input_mode, &color_mode);

  if (verbose_mode) {
    printfv(verbose_mode, color_mode, "", "%sVerbose Mode: ON\n",
            (verbose_mode == VERY_VERBOSE) ? "Very " : "");
    printfv(verbose_mode, color_mode, "", "Color Mode: %s\n",
            color_mode ? "ON" : "OFF");
    printfv(verbose_mode, color_mode, "", "Input %s:\n",
            (input_mode == DIRECTORIES) ? "dirs" : "files");
    for (uint32_t i = 0; i < input_count; ++i) {
      printfv(verbose_mode, color_mode, "", "  %s\n", input[i]);
    }
    printfv(verbose_mode, color_mode, "", "Output file: %s\n", output_file);
  }

  // files is default
  if (input_mode == FILES || input_mode == INPUT_MODE_E_NONE) {
    sorted_files =
        (file_entry_t *)mallocv("sorted_files", verbose_mode, color_mode,
                                input_count * sizeof(file_entry_t), -1);
    // parse files and sort them
    printfv(verbose_mode, color_mode, "", "Handling parsing files\n");
    handle_file_input_parsing(&sorted_files, input, &input_count, &verbose_mode,
                              &color_mode);
    file_count = input_count;
  } else {
    sorted_files = NULL;
    file_count   = 0;

    printfv(verbose_mode, color_mode, "", "Handling parsing dirs\n");
    handle_dir_input_parsing(&sorted_files, input, &input_count, &file_count,
                             &verbose_mode, &color_mode);
  }
  // do the program now lol
  extract_and_combine_cbz((const file_entry_t **)&sorted_files, output_file,
                          &file_count, &verbose_mode, &color_mode);

  free_sorted_files(&sorted_files, &file_count, &verbose_mode, &color_mode);
  free_output_file(&output_file, &verbose_mode, &color_mode);

  return 0;
}
