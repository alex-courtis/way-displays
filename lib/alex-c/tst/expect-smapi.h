#ifndef EXPECT_SMAPI_H
#define EXPECT_SMAPI_H

#include <cmocka.h>
#include <stdbool.h>

#include "util-file.h"

#include "smapi.h"

static int check_smapi_equal(CMockaValueData value, CMockaValueData check_data) {

	const struct SMapI* const actual = (struct SMapI*)value.ptr;
	const struct SMapI* const expected = (struct SMapI*)check_data.ptr;

	if (smapi_equal(actual, expected)) {
		return true;
	} else {
		write_file("actual.smapi", smapi_str(actual));
		write_file("expected.smapi", smapi_str(expected));
		cmocka_print_error("\n%s != \n%s", smapi_str(actual), smapi_str(expected));
		return false;
	}
}

#define expect_smapi(function, parameter, value) \
	expect_check_data(function, parameter, check_smapi_equal, cast_ptr_to_cmocka_value(value))

#endif // EXPECT_SMAPI_H
