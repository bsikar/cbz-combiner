#include "cli.h"
#include "extras.h"
#include "file_entry_t.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void free_input(const cli_flags_t *cli_flags, char **input,
                const uint32_t *input_count) {
  if (input == NULL || input_count == NULL) {
    printfv(*cli_flags, RED,
            "Invalid null pointer argument(s) provided to free_input\n");
    return;
  }
  for (uint32_t i = 0; i < *input_count; ++i) {
    printfv(*cli_flags, BLUE, "Freeing input entry: %s\n", input[i]);
    freev(*cli_flags, input[i], "input", i);
  }
  printfv(*cli_flags, BLUE, "Freeing the input array\n");
  freev(*cli_flags, input, "input", -1);
}

void free_output_file(const cli_flags_t *cli_flags, char **output_file) {
  if (output_file == NULL) {
    printfv(*cli_flags, RED,
            "Invalid null pointer argument(s) provided to free_output_file\n");
    return;
  }
  printfv(*cli_flags, BLUE, "Freeing the output file\n");
  freev(*cli_flags, *output_file, "output_file", -1);
}

void free_memory(const cli_flags_t *cli_flags, char **input,
                 const uint32_t *input_count, char **output_file) {
  free_input(cli_flags, input, input_count);
  free_output_file(cli_flags, output_file);
}

void handle_cli(cli_flags_t *cli_flags, const int *argc, const char **argv,
                uint32_t *input_count, char **output_file, char **input) {
  for (uint32_t i = 1; i < *argc; ++i) {
    if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      print_usage(argv);
      // must free
      free_memory(cli_flags, input, input_count, output_file);
      exit(0);
    } else if (strcmp(argv[i], "-v") == 0 ||
               strcmp(argv[i], "--verbose") == 0) {
      cli_flags->verbose_mode = MAX(cli_flags->verbose_mode, VERBOSE);
    } else if (strcmp(argv[i], "-vv") == 0 ||
               strcmp(argv[i], "--very_verbose") == 0) {
      cli_flags->verbose_mode = VERY_VERBOSE;
    } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
      check_arg(i++, *argc, 2);
      printfv(*cli_flags, BLUE, "Freeing ouput_file: %s\n", *output_file);
      freev(*cli_flags, *output_file, "output_file", -1);
      size_t len   = strlen(argv[i]) + 1;
      *output_file = (char *)mallocv(*cli_flags, "output_file", len, -1);
      strncpyv(*cli_flags, *output_file, argv[i], len, len);
    } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--files") == 0) {
      check_arg(i, *argc, 5);
      if (cli_flags->input_mode == DIRECTORIES) {
        // must free
        free_memory(cli_flags, input, input_count, output_file);
        print_error(4);
      }
      cli_flags->input_mode = FILES;
    } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dirs") == 0) {
      check_arg(i, *argc, 5);
      if (cli_flags->input_mode == FILES) {
        // must free
        free_memory(cli_flags, input, input_count, output_file);
        print_error(4);
      }
      cli_flags->input_mode = DIRECTORIES;
    } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--color") == 0) {
      cli_flags->color_mode = COLOR_ENABLED;
    } else {
      // store the input files or directories
      if (cli_flags->input_mode == INPUT_MODE_E_NONE) {
        cli_flags->input_mode = FILES;
        continue;
      }
      if (cli_flags->input_mode == FILES && !is_file(argv[i])) {
        print_error_s(7, argv[i]);
      } else if (cli_flags->input_mode == DIRECTORIES && !is_dir(argv[i])) {
        print_error_s(8, argv[i]);
      }
      size_t len = strlen(argv[i]) + 1;
      input[*input_count] =
          (char *)mallocv(*cli_flags, "input", len, *input_count);
      if (input[*input_count] == NULL) {
        // must free
        free_memory(cli_flags, input, input_count, output_file);
        print_error(3);
      }
      strncpyv(*cli_flags, input[*input_count], argv[i], len, len);
      ++(*input_count);
    }
  }

  if (*input_count == 0) {
    print_usage(argv);
    // must free
    free_memory(cli_flags, input, input_count, output_file);
    print_error(1);
  }
}
