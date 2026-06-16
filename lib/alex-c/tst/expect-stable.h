#ifndef EXPECT_STABLE_H
#define EXPECT_STABLE_H

#include <cmocka.h>
#include <stdbool.h>

#include "util-file.h"

#include "fn.h"
#include "stable.h"

static int check_stable_equal_strcmp(CMockaValueData value, CMockaValueData check_data) {

	const struct STable* const actual = (struct STable*)value.ptr;
	const struct STable* const expected = (struct STable*)check_data.ptr;

	if (stable_equal(actual, expected)) {
		return true;
	} else {
		write_file("actual.stable", stable_str(actual, fn_str_or_null));
		write_file("expected.ptable", stable_str(expected, fn_str_or_null));
		cmocka_print_error("\n%s != \n%s", stable_str(actual, fn_str_or_null), stable_str(expected, fn_str_or_null));
		return false;
	}
}

#define expect_stable_strcmp(function, parameter, value) \
	expect_check_data(function, parameter, check_stable_equal_strcmp, cast_ptr_to_cmocka_value(value))

#endif // EXPECT_STABLE_H
