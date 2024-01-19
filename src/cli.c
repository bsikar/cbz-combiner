#include "cli.h"
#include "extras.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void free_input(char **input, const uint32_t *input_count,
                const verbose_mode_e *verbose_mode,
                const color_mode_e   *color_mode) {
  if (input == NULL || input_count == NULL || verbose_mode == NULL) {
    printfv(*verbose_mode, *color_mode, RED,
            "Invalid null pointer argument(s) provided to free_input\n");
    return;
  }
  for (uint32_t i = 0; i < *input_count; ++i) {
    printfv(*verbose_mode, *color_mode, BLUE, "Freeing input entry: %s\n",
            input[i]);
    freev(*verbose_mode, *color_mode, input[i], "input", i);
  }
  printfv(*verbose_mode, *color_mode, BLUE, "Freeing the input array\n");
  freev(*verbose_mode, *color_mode, input, "input", -1);
}

void free_output_file(char **output_file, const verbose_mode_e *verbose_mode,
                      const color_mode_e *color_mode) {
  if (output_file == NULL || verbose_mode == NULL) {
    printfv(*verbose_mode, *color_mode, RED,
            "Invalid null pointer argument(s) provided to free_output_file\n");
    return;
  }
  printfv(*verbose_mode, *color_mode, BLUE, "Freeing the output file\n");
  freev(*verbose_mode, *color_mode, *output_file, "output_file", -1);
}

void free_memory(char **input, const uint32_t *input_count, char **output_file,
                 const verbose_mode_e *verbose_mode,
                 const color_mode_e   *color_mode) {
  free_input(input, input_count, verbose_mode, color_mode);
  free_output_file(output_file, verbose_mode, color_mode);
}

void handle_cli(const int *argc, const char **argv, uint32_t *input_count,
                char **output_file, char **input, verbose_mode_e *verbose_mode,
                input_mode_e *input_mode, color_mode_e *color_mode) {
  for (uint32_t i = 1; i < *argc; ++i) {
    if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      print_usage(argv);
      // must free
      free_memory(input, input_count, output_file, verbose_mode, color_mode);
      exit(0);
    } else if (strcmp(argv[i], "-v") == 0 ||
               strcmp(argv[i], "--verbose") == 0) {
      *verbose_mode = MAX(*verbose_mode, VERBOSE);
    } else if (strcmp(argv[i], "-vv") == 0 ||
               strcmp(argv[i], "--very_verbose") == 0) {
      *verbose_mode = VERY_VERBOSE;
    } else if (strcmp(argv[i], "-o") == 0) {
      check_arg(i++, *argc, 2);
      printfv(*verbose_mode, *color_mode, BLUE, "Freeing ouput_file: %s\n",
              *output_file);
      freev(*verbose_mode, *color_mode, *output_file, "output_file", -1);
      *output_file = (char *)mallocv("output_file", *verbose_mode, *color_mode,
                                     strlen(argv[i]) + 1, -1);
      strcpy(*output_file, argv[i]);
    } else if (strcmp(argv[i], "--files") == 0) {
      check_arg(i, *argc, 5);
      if (*input_mode == DIRECTORIES) {
        // must free
        free_memory(input, input_count, output_file, verbose_mode, color_mode);
        print_error(4);
      }
      *input_mode = FILES;
    } else if (strcmp(argv[i], "--dirs") == 0) {
      check_arg(i, *argc, 5);
      if (*input_mode == FILES) {
        // must free
        free_memory(input, input_count, output_file, verbose_mode, color_mode);
        print_error(4);
      }
      *input_mode = DIRECTORIES;
    } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--color") == 0) {
      *color_mode = COLOR_ENABLED;
    } else {
      input[*input_count] = (char *)mallocv("input", *verbose_mode, *color_mode,
                                            strlen(argv[i]) + 1, *input_count);
      if (input[*input_count] == NULL) {
        // must free
        free_memory(input, input_count, output_file, verbose_mode, color_mode);
        print_error(3);
      }
      strcpy(input[*input_count], argv[i]);
      ++(*input_count);
    }
  }

  if (*input_count == 0) {
    print_usage(argv);
    // must free
    free_memory(input, input_count, output_file, verbose_mode, color_mode);
    print_error(1);
  }
}
