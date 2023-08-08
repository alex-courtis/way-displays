#ifndef TST_H
#define TST_H

#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

#include <cmocka.h>

// TODO: use cmocka_print_error and remove this proto once cmocka 1.1.7+ is widely available
void cm_print_error(const char* const format, ...) CMOCKA_PRINTF_ATTRIBUTE(1, 2);

// print log messages, useful when debugging tests
#define LOG_PRINT false

//
// utility
//
char *read_file(const char *path);

void write_file(const char *path, const char *content);

//
// test definition
//
#define TEST(t) cmocka_unit_test_setup_teardown(t, before_each, after_each)
#define RUN(t) cmocka_run_group_tests(t, before_all, after_all)

#endif // TST_H
