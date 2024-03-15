#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { NONE, LEFT, RIGHT } page_type_e;
typedef struct {
  uint32_t    n;
  page_type_e t;
} page_t;

char *t_to_s(page_type_e t);
void  test_case1(void);
void  test_case2(void);
void  test_case3(void);
void  test_case4(void);
void  test_case5(void);
void  test_case6(void);
void  test_case7(void);
void  test_case8(void);
void  test_case9(void);
void  test_case10(void);

#define HUMAN_READABLE_TO_STR(output_human, output_human_len, last_non_null)   \
  ({                                                                           \
    char result_str[200];                                                      \
    if (result_str == NULL) {                                                  \
      exit(EXIT_FAILURE);                                                      \
    }              /* Handle allocation failure */                             \
    char temp[20]; /* Temporary buffer for individual elements */              \
    strcpy(result_str, "[");                                                   \
    for (uint32_t i = 0; i < last_non_null; ++i) {                             \
      if (!output_human[i]) {                                                  \
        strcat(result_str, "X, ");                                             \
        continue;                                                              \
      }                                                                        \
      sprintf(temp, "%u%s, ", output_human[i]->n, t_to_s(output_human[i]->t)); \
      strcat(result_str, temp);                                                \
    }                                                                          \
    for (uint32_t i = last_non_null; i < output_human_len; ++i) {              \
      if (!output_human[i]) {                                                  \
        strcat(result_str, "N, ");                                             \
      } else {                                                                 \
        sprintf(temp, "%u%s, ", output_human[output_human_len - 1]->n,         \
                t_to_s(output_human[output_human_len - 1]->t));                \
        strcat(result_str, temp);                                              \
      }                                                                        \
    }                                                                          \
    /* Remove the last comma and space */                                      \
    size_t len = strlen(result_str);                                           \
    if (len > 2) {                                                             \
      result_str[len - 2] = ']';                                               \
      result_str[len - 1] = '\0';                                              \
    } else {                                                                   \
      strcat(result_str, "]");                                                 \
    }                                                                          \
    result_str;                                                                \
  })

#define OUTPUT_TO_STR(output_human, output_human_len)                          \
  ({                                                                           \
    char result_str[200];                                                      \
    if (result_str == NULL) {                                                  \
      exit(EXIT_FAILURE);                                                      \
    }              /* Handle allocation failure */                             \
    char temp[20]; /* Temporary buffer for individual elements */              \
    strcpy(result_str, "[");                                                   \
    for (uint32_t i = 0; i < output_human_len; ++i) {                          \
      if (!output_human[i]) {                                                  \
        strcat(result_str, "X, ");                                             \
        continue;                                                              \
      }                                                                        \
      sprintf(temp, "%u%s, ", output_human[i]->n, t_to_s(output_human[i]->t)); \
      strcat(result_str, temp);                                                \
    }                                                                          \
    /* Remove the last comma and space */                                      \
    size_t len = strlen(result_str);                                           \
    if (len > 2) {                                                             \
      result_str[len - 2] = ']';                                               \
      result_str[len - 1] = '\0';                                              \
    } else {                                                                   \
      strcat(result_str, "]");                                                 \
    }                                                                          \
    result_str;                                                                \
  })

#define REORDER_TO_HUMAN_READABLE(input, output_human, output_human_len,       \
                                  INPUT_SIZE, last_non_null)                   \
  uint32_t start_index = 0;                                                    \
  /* Check first */                                                            \
  if (input[0].t == NONE) {                                                    \
    output_human[output_human_len++] = &input[0];                              \
    start_index                      = output_human_len;                       \
  } else if (input[0].t == LEFT) {                                             \
    output_human[output_human_len++] = NULL;                                   \
    output_human[output_human_len++] = &input[1];                              \
    output_human[output_human_len++] =                                         \
        &input[0]; /* has to be L, we are ensured */                           \
    start_index = output_human_len - 1;                                        \
  }                                                                            \
                                                                               \
  for (uint32_t i = start_index; i < INPUT_SIZE; ++i) {                        \
    page_t p             = input[i];                                           \
    bool   on_right_page = (output_human_len - 1) % 2 == 0;                    \
    if (p.t == NONE) {                                                         \
      if (on_right_page && i < INPUT_SIZE - 1 && (input[i + 1].t != NONE)) {   \
        output_human[output_human_len++] = NULL;                               \
      }                                                                        \
      output_human[output_human_len++] = &input[i];                            \
    } else {                                                                   \
      if (!on_right_page) { /* on left page_t */                               \
        output_human[output_human_len++] = NULL;                               \
      }                                                                        \
      /* we are ensured that this will be in bounds since L then R */          \
      output_human[output_human_len++] = &input[i + 1]; /* Add Right first */  \
      output_human[output_human_len++] =                                       \
          &input[i++]; /* Add Left second, inc to skip right */                \
    }                                                                          \
  }                                                                            \
  last_non_null = output_human_len;                                            \
  while (output_human_len % 4 != 0) {                                          \
    output_human[output_human_len++] = NULL;                                   \
  }

#define REORDER_HUMAN_TO_OUTPUT(output_human, output_human_len, output)        \
  for (uint32_t i = 0, j = output_human_len - 1; i < j; ++i, --j) {            \
    bool     on_front_side = (i % 2) != 0;                                     \
    uint32_t idx1 = i, idx2 = j;                                               \
                                                                               \
    if (on_front_side) {                                                       \
      idx1 = j;                                                                \
      idx2 = i;                                                                \
    }                                                                          \
    output[output_len++] = output_human[idx1];                                 \
    output[output_len++] = output_human[idx2];                                 \
  }

int main(void) {
  test_case1();
  test_case2();
  test_case3();
  test_case4();
  test_case5();
  test_case6();
  test_case7();
  test_case8();
  test_case9();
  test_case10();

  return 0;
}

char *t_to_s(page_type_e t) {
  if (t == NONE) {
    return "";
  }
  if (t == LEFT) {
    return "L";
  }
  return "R";
}

void test_case1(void) {
  /*
  input: [1, 2, 3L, 3R, 4L, 4R, 5, 6, 7L, 7R]
  output_human: [1, X, 2, 3R, 3L, 4R, 4L, 5, 6, 7R, 7L, N]
  output: [1, N, 7L, X, 2, 7R, 6, 3R, 3L, 5, 4L, 4R]
  */
#define TEST_CASE1_INPUT 10
  page_t input[TEST_CASE1_INPUT] = {
      {1, NONE},  {2, NONE}, {3, LEFT}, {3, RIGHT}, {4, LEFT},
      {4, RIGHT}, {5, NONE}, {6, NONE}, {7, LEFT},  {7, RIGHT}};
  const char *expected_output_human =
      "[1, X, 2, 3R, 3L, 4R, 4L, 5, 6, 7R, 7L, N]";
  page_t  *output_human[100];
  uint32_t output_human_len = 0;
  uint32_t last_non_null    = 0;

  // const char *expected_output = "[1, N, 7L, X, 2, 7R, 6, 3R, 3L, 5, 4L, 4R]";
  const char *expected_output = "[1, X, 7L, X, 2, 7R, 6, 3R, 3L, 5, 4L, 4R]";
  page_t     *output[100];
  uint32_t    output_len = 0;

  /* First order the output_human in human readable form */
  REORDER_TO_HUMAN_READABLE(input, output_human, output_human_len,
                            TEST_CASE1_INPUT, last_non_null);
  assert(strcmp(HUMAN_READABLE_TO_STR(output_human, output_human_len,
                                      last_non_null),
                expected_output_human) == 0);

  /* Second reorder the output in printable form */
  REORDER_HUMAN_TO_OUTPUT(output_human, output_human_len, output);
  assert(strcmp(OUTPUT_TO_STR(output, output_len), expected_output) == 0);
}

void test_case2(void) {
  /*
  input: [1, 2, 3L, 3R, 4L, 4R, 5]
  output_human: [1, X, 2, 3R, 3L, 4R, 4L, 5]
  output: [1, 5, 4L, X, 2, 4R, 3L, 3R]
  */
#define TEST_CASE2_INPUT 7
  page_t      input[TEST_CASE2_INPUT] = {{1, NONE},  {2, NONE}, {3, LEFT},
                                         {3, RIGHT}, {4, LEFT}, {4, RIGHT},
                                         {5, NONE}};
  const char *expected_output_human   = "[1, X, 2, 3R, 3L, 4R, 4L, 5]";
  page_t     *output_human[100];
  uint32_t    output_human_len = 0;
  uint32_t    last_non_null    = 0;

  const char *expected_output = "[1, 5, 4L, X, 2, 4R, 3L, 3R]";
  page_t     *output[100];
  uint32_t    output_len = 0;

  /* First order the output_human in human readable form */
  REORDER_TO_HUMAN_READABLE(input, output_human, output_human_len,
                            TEST_CASE2_INPUT, last_non_null);
  assert(strcmp(HUMAN_READABLE_TO_STR(output_human, output_human_len,
                                      last_non_null),
                expected_output_human) == 0);

  /* Second reorder the output in printable form */
  REORDER_HUMAN_TO_OUTPUT(output_human, output_human_len, output);
  assert(strcmp(OUTPUT_TO_STR(output, output_len), expected_output) == 0);
}

void test_case3(void) {
  /*
  input: [1, 2L, 2R, 3L, 3R]
  output_human: [1, 2R, 2L, 3R, 3L, N, N, N]
  output: [1, N, N, 2R, 2L, N, 3L, 3R]
  */
#define TEST_CASE3_INPUT 5
  page_t input[TEST_CASE2_INPUT] = {
      {1, NONE}, {2, LEFT}, {2, RIGHT}, {3, LEFT}, {3, RIGHT}};
  const char *expected_output_human = "[1, 2R, 2L, 3R, 3L, N, N, N]";
  page_t     *output_human[100];
  uint32_t    output_human_len = 0;
  uint32_t    last_non_null    = 0;

  const char *expected_output = "[1, X, X, 2R, 2L, X, 3L, 3R]";
  page_t     *output[100];
  uint32_t    output_len = 0;

  /* First order the output_human in human readable form */
  REORDER_TO_HUMAN_READABLE(input, output_human, output_human_len,
                            TEST_CASE3_INPUT, last_non_null);
  assert(strcmp(HUMAN_READABLE_TO_STR(output_human, output_human_len,
                                      last_non_null),
                expected_output_human) == 0);

  /* Second reorder the output in printable form */
  REORDER_HUMAN_TO_OUTPUT(output_human, output_human_len, output);
  assert(strcmp(OUTPUT_TO_STR(output, output_len), expected_output) == 0);
}

void test_case4(void) {
  // dont have a case for this right now
}

void test_case5(void) {
  /*
  input: [1, 2L, 2R, 3, 4L, 4R]
  output_human: [1, 2R, 2L, X, 3, 4R, 4L, N]
  output: [1, N, 4L, 2R, 2L, 4R, 3, X]
  */
#define TEST_CASE5_INPUT 6
  page_t      input[TEST_CASE5_INPUT] = {{1, NONE}, {2, LEFT}, {2, RIGHT},
                                         {3, NONE}, {4, LEFT}, {4, RIGHT}};
  const char *expected_output_human   = "[1, 2R, 2L, X, 3, 4R, 4L, N]";
  page_t     *output_human[100];
  uint32_t    output_human_len = 0;
  uint32_t    last_non_null    = 0;

  const char *expected_output = "[1, X, 4L, 2R, 2L, 4R, 3, X]";
  page_t     *output[100];
  uint32_t    output_len = 0;

  /* First order the output_human in human readable form */
  REORDER_TO_HUMAN_READABLE(input, output_human, output_human_len,
                            TEST_CASE5_INPUT, last_non_null);
  assert(strcmp(HUMAN_READABLE_TO_STR(output_human, output_human_len,
                                      last_non_null),
                expected_output_human) == 0);

  /* Second reorder the output in printable form */
  REORDER_HUMAN_TO_OUTPUT(output_human, output_human_len, output);
  assert(strcmp(OUTPUT_TO_STR(output, output_len), expected_output) == 0);
}
void test_case6(void) {
  /*
  input: [1L, 1R, 2, 3L, 3R]
  output_human: [X, 1R, 1L, X, 2, 3R, 3L, N]
  output: [X, N, 3L, 1R, 1L, 3R, 2, X]
  */
#define TEST_CASE6_INPUT 5
  page_t input[TEST_CASE6_INPUT] = {
      {1, LEFT}, {1, RIGHT}, {2, NONE}, {3, LEFT}, {3, RIGHT}};
  const char *expected_output_human = "[X, 1R, 1L, X, 2, 3R, 3L, N]";
  page_t     *output_human[100];
  uint32_t    output_human_len = 0;
  uint32_t    last_non_null    = 0;

  const char *expected_output = "[X, X, 3L, 1R, 1L, 3R, 2, X]";
  page_t     *output[100];
  uint32_t    output_len = 0;

  /* First order the output_human in human readable form */
  REORDER_TO_HUMAN_READABLE(input, output_human, output_human_len,
                            TEST_CASE6_INPUT, last_non_null);
  assert(strcmp(HUMAN_READABLE_TO_STR(output_human, output_human_len,
                                      last_non_null),
                expected_output_human) == 0);

  /* Second reorder the output in printable form */
  REORDER_HUMAN_TO_OUTPUT(output_human, output_human_len, output);
  assert(strcmp(OUTPUT_TO_STR(output, output_len), expected_output) == 0);
}
void test_case7(void) {
  /*
  input: [1L, 1R, 2, 3L, 3R, 4]
  output_human: [X, 1R, 1L, X, 2, 3R, 3L, 4]
  output: [X, 4, 3L, 1R, 1L, 3R, 2, X]
  */
#define TEST_CASE7_INPUT 6
  page_t      input[TEST_CASE7_INPUT] = {{1, LEFT}, {1, RIGHT}, {2, NONE},
                                         {3, LEFT}, {3, RIGHT}, {4, NONE}};
  const char *expected_output_human   = "[X, 1R, 1L, X, 2, 3R, 3L, 4]";
  page_t     *output_human[100];
  uint32_t    output_human_len = 0;
  uint32_t    last_non_null    = 0;

  const char *expected_output = "[X, 4, 3L, 1R, 1L, 3R, 2, X]";
  page_t     *output[100];
  uint32_t    output_len = 0;

  /* First order the output_human in human readable form */
  REORDER_TO_HUMAN_READABLE(input, output_human, output_human_len,
                            TEST_CASE7_INPUT, last_non_null);
  assert(strcmp(HUMAN_READABLE_TO_STR(output_human, output_human_len,
                                      last_non_null),
                expected_output_human) == 0);

  /* Second reorder the output in printable form */
  REORDER_HUMAN_TO_OUTPUT(output_human, output_human_len, output);
  assert(strcmp(OUTPUT_TO_STR(output, output_len), expected_output) == 0);
}
void test_case8(void) {
  /*
  input: [1L, 1R, 2, 3, 4L, 4R, 5, 6]
  output_human: [X, 1R, 1L, 2, 3, 4R, 4L, 5, 6, N, N, N]
  output: [X, N, N, 1R, 1L, N, 6, 2, 3, 5, 4L, 4R]
  */
#define TEST_CASE8_INPUT 8
  page_t      input[TEST_CASE8_INPUT] = {{1, LEFT}, {1, RIGHT}, {2, NONE},
                                         {3, NONE}, {4, LEFT},  {4, RIGHT},
                                         {5, NONE}, {6, NONE}};
  const char *expected_output_human =
      "[X, 1R, 1L, 2, 3, 4R, 4L, 5, 6, N, N, N]";
  page_t  *output_human[100];
  uint32_t output_human_len = 0;
  uint32_t last_non_null    = 0;

  const char *expected_output = "[X, X, X, 1R, 1L, X, 6, 2, 3, 5, 4L, 4R]";
  page_t     *output[100];
  uint32_t    output_len = 0;

  /* First order the output_human in human readable form */
  REORDER_TO_HUMAN_READABLE(input, output_human, output_human_len,
                            TEST_CASE8_INPUT, last_non_null);
  assert(strcmp(HUMAN_READABLE_TO_STR(output_human, output_human_len,
                                      last_non_null),
                expected_output_human) == 0);

  /* Second reorder the output in printable form */
  REORDER_HUMAN_TO_OUTPUT(output_human, output_human_len, output);
  assert(strcmp(OUTPUT_TO_STR(output, output_len), expected_output) == 0);
}
void test_case9(void) {
  /*
  input: [1, 2, 3, 4L, 4R, 5, 6, 7]
  output_human: [1, 2, 3, 4R, 4L, 5, 6, 7]
  output: [1, 7, 6, 2, 3, 5, 4L, 4R]
  */
#define TEST_CASE9_INPUT 8
  page_t      input[TEST_CASE9_INPUT] = {{1, NONE}, {2, NONE},  {3, NONE},
                                         {4, LEFT}, {4, RIGHT}, {5, NONE},
                                         {6, NONE}, {7, NONE}};
  const char *expected_output_human   = "[1, 2, 3, 4R, 4L, 5, 6, 7]";
  page_t     *output_human[100];
  uint32_t    output_human_len = 0;
  uint32_t    last_non_null    = 0;

  const char *expected_output = "[1, 7, 6, 2, 3, 5, 4L, 4R]";
  page_t     *output[100];
  uint32_t    output_len = 0;

  /* First order the output_human in human readable form */
  REORDER_TO_HUMAN_READABLE(input, output_human, output_human_len,
                            TEST_CASE9_INPUT, last_non_null);
  assert(strcmp(HUMAN_READABLE_TO_STR(output_human, output_human_len,
                                      last_non_null),
                expected_output_human) == 0);

  /* Second reorder the output in printable form */
  REORDER_HUMAN_TO_OUTPUT(output_human, output_human_len, output);
  assert(strcmp(OUTPUT_TO_STR(output, output_len), expected_output) == 0);
}
void test_case10(void) {
  /*
  input: [1, 2, 3L, 3R, 4, 5, 6L, 6R]
  output_human: [1, X, 2, 3R, 3L, 4, 5, 6R, 6L, N, N, N]
  output: [1, N, N, X, 2, N, 6L, 3R, 3L, 6R, 5, 4]
  */
#define TEST_CASE10_INPUT 8
  page_t      input[TEST_CASE10_INPUT] = {{1, NONE},  {2, NONE}, {3, LEFT},
                                          {3, RIGHT}, {4, NONE}, {5, NONE},
                                          {6, LEFT},  {6, RIGHT}};
  const char *expected_output_human =
      "[1, X, 2, 3R, 3L, 4, 5, 6R, 6L, N, N, N]";
  page_t  *output_human[100];
  uint32_t output_human_len = 0;
  uint32_t last_non_null    = 0;

  const char *expected_output = "[1, X, X, X, 2, X, 6L, 3R, 3L, 6R, 5, 4]";
  page_t     *output[100];
  uint32_t    output_len = 0;

  /* First order the output_human in human readable form */
  REORDER_TO_HUMAN_READABLE(input, output_human, output_human_len,
                            TEST_CASE10_INPUT, last_non_null);
  assert(strcmp(HUMAN_READABLE_TO_STR(output_human, output_human_len,
                                      last_non_null),
                expected_output_human) == 0);

  /* Second reorder the output in printable form */
  REORDER_HUMAN_TO_OUTPUT(output_human, output_human_len, output);
  assert(strcmp(OUTPUT_TO_STR(output, output_len), expected_output) == 0);
}
