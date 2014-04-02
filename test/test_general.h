#ifndef TEST_GENERAL_H
#define TEST_GENERAL_H

#include <string.h>
#include <stdint.h> /* for unint*_t */

extern void test_pass(void);
extern void test_fail(const char *system_message);
extern void test_print_stats(void);

extern void assert_equal_memory(const void *expected,
                                size_t      expected_size,
                                const void *actual,
                                size_t      actual_size,
                                const char *assert_message
                                );

extern void assert_equal_int(long        expected, 
                             long        actual,
                             const char *assert_message);


extern void assert_equal_bool(uint8_t     expected,
                              uint8_t     actual,
                              const char *assert_message);

#endif                                                      /* TEST_GENERAL_H */
