#ifndef EXTRAS_H
#define EXTRAS_H

#include <stdio.h>
#include <stdlib.h>

extern const char *error_messages[];

typedef enum {
  VERBOSE_MODE_E_NONE,
  VERBOSE,
  VERY_VERBOSE,
} verbose_mode_e;

typedef enum {
  COLOR_DISABLED,
  COLOR_ENABLED,
} color_mode_e;

// Regular color definitions
#define RED    "\033[38;5;9m"
#define GREEN  "\033[38;5;10m"
#define BLUE   "\033[38;5;12m"
#define ORANGE "\033[38;5;214m" // Orange is typically bright by default
#define YELLOW "\033[38;5;11m"

// Light color definitions
#define LIGHT_RED    "\033[38;5;196m"
#define LIGHT_GREEN  "\033[38;5;82m"
#define LIGHT_BLUE   "\033[38;5;21m"
#define LIGHT_ORANGE "\033[38;5;220m"
#define LIGHT_YELLOW "\033[38;5;226m"

// Dark color definitions
#define DARK_RED    "\033[38;5;1m"
#define DARK_GREEN  "\033[38;5;2m"
#define DARK_BLUE   "\033[38;5;4m"
#define DARK_ORANGE "\033[38;5;208m"
#define DARK_YELLOW "\033[38;5;3m"

#define RESET "\033[0m"

#define DEFAULT_OUTPUT_FILE_NAME "combined_output.cbz"
#define CBZ                      ".cbz"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define print_error(code)                                                      \
  do {                                                                         \
    fprintf(stderr, "Error: %s\n", error_messages[code]);                      \
    exit(code);                                                                \
  } while (0)

// clang-format off
#define print_usage(argv)                                                                                   \
  do {                                                                                                      \
    printf(                                                                                                 \
        "Usage: %s [color option] [verbose option] [rest of the options] <file1.cbz> [file2.cbz] ...\n"     \
        "Options:\n"                                                                                        \
        "  -h,  --help          Display this help and exit\n"                                               \
        "  -v,  --verbose       Verbose mode\n"                                                             \
        "  -vv, --very_verbose  Very Verbose mode\n"                                                        \
        "  -f,  --files         List all .cbz files (cannot be used with --dirs)\n"                         \
        "  -d,  --dirs          List all dirs (cannot be used with --files)\n"                              \
        "  -o,  --output        Specify output file (default is combined.cbz)\n"                            \
        "  -c, --color          Specify output to use color\n"                                              \
        "\nBecause of how the cli is parsed color then verbose options should go first (for good logs)\n",  \
        argv[0]);                                                                                           \
  } while (0)
// clang-format on

#define check_arg(i, argc, code)                                               \
  do {                                                                         \
    if ((i) + 1 >= (argc)) {                                                   \
      print_error(code);                                                       \
    }                                                                          \
  } while (0)

#define printfv(verbose_mode, color_mode, cc, ...)                             \
  do {                                                                         \
    if (color_mode) {                                                          \
      printf("%s", cc);                                                        \
    }                                                                          \
    if (verbose_mode == VERY_VERBOSE) {                                        \
      printf("In function <%s> | ", __func__);                                 \
      printf(__VA_ARGS__);                                                     \
    } else if (verbose_mode == VERBOSE) {                                      \
      printf(__VA_ARGS__);                                                     \
    }                                                                          \
    if (color_mode) {                                                          \
      printf(RESET);                                                           \
    }                                                                          \
  } while (0)

#define freev(verbose_mode, color_mode, ptr, ptr_name, index)                  \
  do {                                                                         \
    if (ptr != NULL) {                                                         \
      if (verbose_mode == VERY_VERBOSE) {                                      \
        if (index != -1) {                                                     \
          printfv(verbose_mode, color_mode, DARK_ORANGE,                       \
                  "Trying to free <%s[%d] (%p)>\n", ptr_name, index,           \
                  (void *)ptr);                                                \
        } else {                                                               \
          printfv(verbose_mode, color_mode, DARK_ORANGE,                       \
                  "Trying to free <%s (%p)>\n", ptr_name, (void *)ptr);        \
        }                                                                      \
      }                                                                        \
      free(ptr);                                                               \
      ptr = NULL;                                                              \
      if (verbose_mode == VERY_VERBOSE) {                                      \
        if (index != -1) {                                                     \
          printfv(verbose_mode, color_mode, DARK_GREEN,                        \
                  "Freed <%s[%d] (%p)>\n", ptr_name, index, (void *)ptr);      \
        } else {                                                               \
          printfv(verbose_mode, color_mode, DARK_GREEN, "Freed <%s (%p)>\n",   \
                  ptr_name, (void *)ptr);                                      \
        }                                                                      \
      }                                                                        \
    } else if (verbose_mode == VERY_VERBOSE) {                                 \
      if (index != -1) {                                                       \
        printfv(verbose_mode, color_mode, RED,                                 \
                "Could not free <%s[%d] (%p)> it is null\n", ptr_name, index,  \
                (void *)ptr);                                                  \
      } else {                                                                 \
        printfv(verbose_mode, color_mode, RED,                                 \
                "Could not free <%s (%p)> it is null\n", ptr_name,             \
                (void *)ptr);                                                  \
      }                                                                        \
    }                                                                          \
  } while (0)

#define mallocv(ptr_name, verbose_mode, color_mode, size, index)               \
  ({                                                                           \
    void *ptr = malloc(size);                                                  \
    if (verbose_mode == VERY_VERBOSE) {                                        \
      if (index != -1) {                                                       \
        printfv(verbose_mode, color_mode, DARK_YELLOW,                         \
                "Attempting to allocate %zu bytes to <%s[%d] (%p)>\n", size,   \
                ptr_name, index, ptr);                                         \
      } else {                                                                 \
        printfv(verbose_mode, color_mode, DARK_YELLOW,                         \
                "Attempting to allocate %zu bytes to <%s (%p)>\n", size,       \
                ptr_name, ptr);                                                \
      }                                                                        \
    }                                                                          \
    if (ptr == NULL) {                                                         \
      if (verbose_mode == VERY_VERBOSE) {                                      \
        if (index != -1) {                                                     \
          printfv(verbose_mode, color_mode, RED,                               \
                  "Failed to allocate %zu bytes to <%s[%d] (%p)>\n", size,     \
                  ptr_name, index, ptr);                                       \
        } else {                                                               \
          printfv(verbose_mode, color_mode, RED,                               \
                  "Failed to allocate %zu bytes to <%s (%p)>\n", size,         \
                  ptr_name, ptr);                                              \
        }                                                                      \
      }                                                                        \
    } else {                                                                   \
      if (verbose_mode == VERY_VERBOSE) {                                      \
        if (index != -1) {                                                     \
          printfv(verbose_mode, color_mode, DARK_GREEN,                        \
                  "Allocated %zu bytes to <%s[%d] (%p)>\n", size, ptr_name,    \
                  index, ptr);                                                 \
        } else {                                                               \
          printfv(verbose_mode, color_mode, DARK_GREEN,                        \
                  "Allocated %zu bytes to <%s (%p)>\n", size, ptr_name, ptr);  \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    ptr;                                                                       \
  })

#define reallocv(verbose_mode, color_mode, ptr, ptr_name, size, index)         \
  ({                                                                           \
    void  *old_ptr  = (ptr);                                                   \
    size_t new_size = (size);                                                  \
    void  *new_ptr  = realloc(old_ptr, new_size);                              \
    if (verbose_mode == VERY_VERBOSE) {                                        \
      if (index != -1) {                                                       \
        printfv(verbose_mode, color_mode, DARK_ORANGE,                         \
                "Trying to reallocate <%s[%d] (%p)> to %zu bytes at (%p)\n",   \
                ptr_name, index, old_ptr, new_size, new_ptr);                  \
      } else {                                                                 \
        printfv(verbose_mode, color_mode, DARK_ORANGE,                         \
                "Trying to reallocate <%s (%p)> to %zu bytes at (%p)\n",       \
                ptr_name, old_ptr, new_size, new_ptr);                         \
      }                                                                        \
      if (new_ptr == NULL) {                                                   \
        if (index != -1) {                                                     \
          printfv(verbose_mode, color_mode, RED,                               \
                  "Could not reallocate <%s[%d] (%p)> - keeping the old "      \
                  "pointer\n",                                                 \
                  ptr_name, index, old_ptr);                                   \
        } else {                                                               \
          printfv(                                                             \
              verbose_mode, color_mode, RED,                                   \
              "Could not reallocate <%s (%p)> - keeping the old pointer\n",    \
              ptr_name, old_ptr);                                              \
        }                                                                      \
      } else {                                                                 \
        if (index != -1) {                                                     \
          printfv(verbose_mode, color_mode, DARK_GREEN,                        \
                  "Reallocated <%s[%d] (%p)> to %zu bytes at (%p)\n",          \
                  ptr_name, index, old_ptr, new_size, new_ptr);                \
        } else {                                                               \
          printfv(verbose_mode, color_mode, DARK_GREEN,                        \
                  "Reallocated <%s (%p)> to %zu bytes at (%p)\n", ptr_name,    \
                  old_ptr, new_size, new_ptr);                                 \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    new_ptr ? new_ptr : old_ptr;                                               \
  })

#endif // EXTRAS_H
