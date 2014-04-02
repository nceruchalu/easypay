
#include "test_general.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


/* functions local to this file */

static int passed_tests = 0;
static int failed_tests = 0;

void test_pass(void)
{
  ++passed_tests;
}

void test_fail(const char *system_message)
{
  printf("%s\n", system_message);
  ++failed_tests;
}

void test_print_stats(void)
{
  printf("passed: %d; failed: %d\n", passed_tests, failed_tests);
}


void assert_equal_memory(const void *expected,
                         size_t      expected_size,
                         const void *actual,
                         size_t      actual_size,
                         const char  *assert_message
                         )
{
  if ((expected_size == actual_size) && 
      (memcmp(expected, actual, expected_size) == 0)) {
    test_pass();
  } else {
    test_fail(assert_message);
  }
}

void assert_equal_int(long expected,
                      long actual,
                      const char *assert_message)
{
  if (expected == actual) {
    test_pass();
  } else {
    test_fail(assert_message);
  }
}

void assert_equal_bool(uint8_t expected,
                       uint8_t actual,
                       const char *assert_message)
{
  if (!(expected ^ actual)) {
    test_pass();
  } else {
    test_fail(assert_message);
  }
}
