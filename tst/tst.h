#ifndef TST_H
#define TST_H

#include <setjmp.h> // IWYU pragma: keep
#include <stdarg.h> // IWYU pragma: keep
#include <string.h>

#include <cmocka.h>

//
// test definition
//
#define TEST(t) cmocka_unit_test_setup_teardown(t, before_each, after_each)
#define RUN(t) cmocka_run_group_tests(t, before_all, after_all)

//
// asserts
//

#endif // TST_H
