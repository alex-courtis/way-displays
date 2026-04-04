#ifndef TST_H
#define TST_H

#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

#include <cmocka.h>

// print log messages, useful when debugging tests
#define LOG_PRINT false

//
// test definition
//
#define TEST(t) cmocka_unit_test_setup_teardown(t, before_each, after_each)
#define RUN(t) cmocka_run_group_tests(t, before_all, after_all)

#endif // TST_H
