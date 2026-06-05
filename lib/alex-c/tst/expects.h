#ifndef EXPECTS_H
#define EXPECTS_H

#include <cmocka.h>
#include <stdbool.h>
#include <string.h>

#include "fn.h"
#include "stable.h"

int check_ptr_equal(CMockaValueData value, CMockaValueData check_data) {
	if (value.ptr != check_data.ptr) {
		cmocka_print_error("%p != %p\n", value.ptr, check_data.ptr);
		return false;
	} else {
		return true;
	}
}

#define expect_ptr(function, parameter, value) \
	expect_check_data(function, parameter, check_ptr_equal, cast_ptr_to_cmocka_value(value));

#define expect_nul(function, parameter) \
	expect_check_data(function, parameter, check_ptr_equal, cast_ptr_to_cmocka_value(NULL));

int check_str_equal(CMockaValueData value, CMockaValueData check_data) {
	char *actual = value.ptr;
	char *expected = check_data.ptr;

	if (!actual && !expected) {
		return true;
	} else if (!actual) {
		cmocka_print_error("%p != \"%s\"\n", actual, expected);
		return false;
	} else if (!expected) {
		cmocka_print_error("\"%s\" != %p\n", actual, expected);
		return false;
	} else if (strcmp(actual, expected) != 0) {
		cmocka_print_error("\"%s\" != \"%s\"\n", actual, expected);
		return false;
	} else {
		return true;
	}
}

#define expect_str(function, parameter, value) \
	expect_check_data(function, parameter, check_str_equal, cast_ptr_to_cmocka_value(value));

int check_stable_equal_strcmp(CMockaValueData value, CMockaValueData check_data) {

	const struct STable* const actual = (struct STable*)value.ptr;
	const struct STable* const expected = (struct STable*)check_data.ptr;

	if (stable_equal(actual, expected, fn_equal_strcmp)) {
		return true;
	} else {
		cmocka_print_error("check_stable_equal_strcmp\nEXPECTED:\n%s\n!=\nACTUAL:\n%s\n", stable_str(expected), stable_str(actual));
		return false;
	}
}

#define expect_stable_equal_strcmp(function, parameter, value) \
	expect_check_data(function, parameter, check_stable_equal_strcmp, cast_ptr_to_cmocka_value(value));

#endif // EXPECTS_H
