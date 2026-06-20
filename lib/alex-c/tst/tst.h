#ifndef TST_H
#define TST_H

#include <cmocka.h>
#include <stddef.h>

//
// test definition
//
#define TEST(t) cmocka_unit_test(t)
#define TEST_A(t) cmocka_unit_test_setup_teardown(t, NULL, after_each)
#define TEST_B(t) cmocka_unit_test_setup_teardown(t, before_each, NULL)
#define TEST_BA(t) cmocka_unit_test_setup_teardown(t, before_each, after_each)

#define RUN(t) cmocka_run_group_tests(t, NULL, NULL)
#define RUN_A(t) cmocka_run_group_tests(t, NULL, after_all)
#define RUN_B(t) cmocka_run_group_tests(t, before_all, NULL)
#define RUN_BA(t) cmocka_run_group_tests(t, before_all, after_all)

#endif // TST_H
