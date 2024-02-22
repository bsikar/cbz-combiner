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

typedef enum {
  ROTATE_DISABLED,
  ROTATE_ENABLED,
} rotate_mode_e;

typedef enum {
  FLIP_DISABLED,
  FLIP_ENABLED,
} flip_mode_e;

typedef enum {
  DOUBLE_PAGE_FALSE,
  DOUBLE_PAGE_TRUE,
  DOUBLE_PAGE_FIRST,
  DOUBLE_PAGE_SECOND,
} double_page_mode_e;

// colors
#define RED         "\033[38;5;9m"
#define BLUE        "\033[38;5;12m"
#define DARK_GREEN  "\033[38;5;2m"
#define DARK_ORANGE "\033[38;5;208m"
#define DARK_YELLOW "\033[38;5;3m"
#define RESET       "\033[0m"

#define DEFAULT_OUTPUT_FILE_NAME "combined_output.cbz"
#define CBZ                      ".cbz"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define print_error_s(code, s)                                                 \
  do {                                                                         \
    fprintf(stderr, "Error: %s ", error_messages[code]);                       \
    fprintf(stderr, "%s\n", s);                                                \
    exit(code);                                                                \
  } while (0)

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
        "  -r, --rotate         If a page on its side it will rotate and resize it\n"                       \
        "  -p, --flip           Flip the pages so they can be printed to view right to left\n"              \
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

#define printfv(cli_flags, cc, ...)                                            \
  do {                                                                         \
    if ((cli_flags).color_mode) {                                              \
      printf("%s", cc);                                                        \
    }                                                                          \
    if ((cli_flags).verbose_mode == VERY_VERBOSE) {                            \
      printf("In function <%s> | ", __func__);                                 \
      printf(__VA_ARGS__);                                                     \
    } else if ((cli_flags).verbose_mode == VERBOSE) {                          \
      printf(__VA_ARGS__);                                                     \
    }                                                                          \
    if ((cli_flags).color_mode) {                                              \
      printf(RESET);                                                           \
    }                                                                          \
  } while (0)

#define strncpyv(cli_flags, dest, src, n, dest_size)                           \
  do {                                                                         \
    if ((cli_flags).verbose_mode == VERY_VERBOSE) {                            \
      printfv(cli_flags, DARK_ORANGE, "Trying to copy string to %p\n",         \
              (void *)dest);                                                   \
    }                                                                          \
    strncpy(dest, src, n);                                                     \
    if ((cli_flags).verbose_mode == VERY_VERBOSE) {                            \
      if ((dest)[dest_size - 1] == '\0') {                                     \
        printfv(cli_flags, DARK_GREEN, "Successfully copied string to %p\n",   \
                (void *)dest);                                                 \
      } else {                                                                 \
        printfv(cli_flags, RED, "String copy to %p may be truncated\n",        \
                (void *)dest);                                                 \
      }                                                                        \
    }                                                                          \
    (dest)[dest_size - 1] = '\0';                                              \
  } while (0)

#define freev(cli_flags, ptr, ptr_name, index)                                 \
  do {                                                                         \
    if (ptr != NULL) {                                                         \
      if ((cli_flags).verbose_mode == VERY_VERBOSE) {                          \
        if ((long)index != -1) {                                               \
          printfv((cli_flags), DARK_ORANGE, "Trying to free <%s[%d] (%p)>\n",  \
                  ptr_name, index, (void *)ptr);                               \
        } else {                                                               \
          printfv((cli_flags), DARK_ORANGE, "Trying to free <%s (%p)>\n",      \
                  ptr_name, (void *)ptr);                                      \
        }                                                                      \
      }                                                                        \
      free(ptr);                                                               \
      ptr = NULL;                                                              \
      if ((cli_flags).verbose_mode == VERY_VERBOSE) {                          \
        if ((long)index != -1) {                                               \
          printfv((cli_flags), DARK_GREEN, "Freed <%s[%d] (%p)>\n", ptr_name,  \
                  index, (void *)ptr);                                         \
        } else {                                                               \
          printfv((cli_flags), DARK_GREEN, "Freed <%s (%p)>\n", ptr_name,      \
                  (void *)ptr);                                                \
        }                                                                      \
      }                                                                        \
    } else if ((cli_flags).verbose_mode == VERY_VERBOSE) {                     \
      if ((long)index != -1) {                                                 \
        printfv((cli_flags), RED, "Could not free <%s[%d] (%p)> it is null\n", \
                ptr_name, index, (void *)ptr);                                 \
      } else {                                                                 \
        printfv((cli_flags), RED, "Could not free <%s (%p)> it is null\n",     \
                ptr_name, (void *)ptr);                                        \
      }                                                                        \
    }                                                                          \
  } while (0)

#define mallocv(cli_flags, ptr_name, size, index)                              \
  ({                                                                           \
    void *ptr = malloc(size);                                                  \
    if ((cli_flags).verbose_mode == VERY_VERBOSE) {                            \
      if ((long)index != -1) {                                                 \
        printfv((cli_flags), DARK_YELLOW,                                      \
                "Attempting to allocate %zu bytes to <%s[%d] (%p)>\n", size,   \
                ptr_name, index, ptr);                                         \
      } else {                                                                 \
        printfv((cli_flags), DARK_YELLOW,                                      \
                "Attempting to allocate %zu bytes to <%s (%p)>\n", size,       \
                ptr_name, ptr);                                                \
      }                                                                        \
    }                                                                          \
    if (ptr == NULL) {                                                         \
      if ((cli_flags).verbose_mode == VERY_VERBOSE) {                          \
        if ((long)index != -1) {                                               \
          printfv((cli_flags), RED,                                            \
                  "Failed to allocate %zu bytes to <%s[%d] (%p)>\n", size,     \
                  ptr_name, index, ptr);                                       \
        } else {                                                               \
          printfv((cli_flags), RED,                                            \
                  "Failed to allocate %zu bytes to <%s (%p)>\n", size,         \
                  ptr_name, ptr);                                              \
        }                                                                      \
      }                                                                        \
    } else {                                                                   \
      if ((cli_flags).verbose_mode == VERY_VERBOSE) {                          \
        if ((long)index != -1) {                                               \
          printfv((cli_flags), DARK_GREEN,                                     \
                  "Allocated %zu bytes to <%s[%d] (%p)>\n", size, ptr_name,    \
                  index, ptr);                                                 \
        } else {                                                               \
          printfv((cli_flags), DARK_GREEN,                                     \
                  "Allocated %zu bytes to <%s (%p)>\n", size, ptr_name, ptr);  \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    ptr;                                                                       \
  })

#define callocv(cli_flags, ptr_name, size1, size2, index)                      \
  ({                                                                           \
    void  *ptr  = calloc(size1, size2);                                        \
    size_t size = size1 * size2;                                               \
    if ((cli_flags).verbose_mode == VERY_VERBOSE) {                            \
      if ((long)index != -1) {                                                 \
        printfv((cli_flags), DARK_YELLOW,                                      \
                "Attempting to allocate %zu bytes to <%s[%d] (%p)>\n", size,   \
                ptr_name, index, ptr);                                         \
      } else {                                                                 \
        printfv((cli_flags), DARK_YELLOW,                                      \
                "Attempting to allocate %zu bytes to <%s (%p)>\n", size,       \
                ptr_name, ptr);                                                \
      }                                                                        \
    }                                                                          \
    if (ptr == NULL) {                                                         \
      if ((cli_flags).verbose_mode == VERY_VERBOSE) {                          \
        if ((long)index != -1) {                                               \
          printfv((cli_flags), RED,                                            \
                  "Failed to allocate %zu bytes to <%s[%d] (%p)>\n", size,     \
                  ptr_name, index, ptr);                                       \
        } else {                                                               \
          printfv((cli_flags), RED,                                            \
                  "Failed to allocate %zu bytes to <%s (%p)>\n", size,         \
                  ptr_name, ptr);                                              \
        }                                                                      \
      }                                                                        \
    } else {                                                                   \
      if ((cli_flags).verbose_mode == VERY_VERBOSE) {                          \
        if ((long)index != -1) {                                               \
          printfv((cli_flags), DARK_GREEN,                                     \
                  "Allocated %zu bytes to <%s[%d] (%p)>\n", size, ptr_name,    \
                  index, ptr);                                                 \
        } else {                                                               \
          printfv((cli_flags), DARK_GREEN,                                     \
                  "Allocated %zu bytes to <%s (%p)>\n", size, ptr_name, ptr);  \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    ptr;                                                                       \
  })

#define reallocv(cli_flags, ptr, ptr_name, size, index)                        \
  ({                                                                           \
    void  *old_ptr  = (ptr);                                                   \
    size_t new_size = (size);                                                  \
    void  *new_ptr  = realloc(old_ptr, new_size);                              \
    if ((cli_flags).verbose_mode == VERY_VERBOSE) {                            \
      if ((long)index != -1) {                                                 \
        printfv((cli_flags), DARK_ORANGE,                                      \
                "Trying to reallocate <%s[%d] (%p)> to %zu bytes at (%p)\n",   \
                ptr_name, index, old_ptr, new_size, new_ptr);                  \
      } else {                                                                 \
        printfv((cli_flags), DARK_ORANGE,                                      \
                "Trying to reallocate <%s (%p)> to %zu bytes at (%p)\n",       \
                ptr_name, old_ptr, new_size, new_ptr);                         \
      }                                                                        \
      if (new_ptr == NULL) {                                                   \
        if ((long)index != -1) {                                               \
          printfv((cli_flags), RED,                                            \
                  "Could not reallocate <%s[%d] (%p)> - keeping the old "      \
                  "pointer\n",                                                 \
                  ptr_name, index, old_ptr);                                   \
        } else {                                                               \
          printfv(                                                             \
              (cli_flags), RED,                                                \
              "Could not reallocate <%s (%p)> - keeping the old pointer\n",    \
              ptr_name, old_ptr);                                              \
        }                                                                      \
      } else {                                                                 \
        if ((long)index != -1) {                                               \
          printfv((cli_flags), DARK_GREEN,                                     \
                  "Reallocated <%s[%d] (%p)> to %zu bytes at (%p)\n",          \
                  ptr_name, index, old_ptr, new_size, new_ptr);                \
        } else {                                                               \
          printfv((cli_flags), DARK_GREEN,                                     \
                  "Reallocated <%s (%p)> to %zu bytes at (%p)\n", ptr_name,    \
                  old_ptr, new_size, new_ptr);                                 \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    new_ptr ? new_ptr : old_ptr;                                               \
  })

#define memcpyv(cli_flags, dest, src, num, dest_size, dest_name)               \
  ({                                                                           \
    void       *d  = (dest);                                                   \
    const void *s  = (src);                                                    \
    size_t      n  = (num);                                                    \
    size_t      ds = (dest_size);                                              \
    if (ds < n) {                                                              \
      if ((cli_flags).verbose_mode == VERY_VERBOSE) {                          \
        printfv((cli_flags), RED,                                              \
                "Error: Destination buffer <%s (%p)> is too small for %zu "    \
                "bytes.\n",                                                    \
                dest_name, d, n);                                              \
      }                                                                        \
    } else {                                                                   \
      memcpy(d, s, n);                                                         \
      if ((cli_flags).verbose_mode == VERY_VERBOSE) {                          \
        printfv((cli_flags), DARK_GREEN,                                       \
                "Copied %zu bytes from (%p) to <%s (%p)>.\n", n, s, dest_name, \
                d);                                                            \
      }                                                                        \
    }                                                                          \
    d;                                                                         \
  })

#endif // EXTRAS_H
