#ifndef EXPECT_SSMAP_H
#define EXPECT_SSMAP_H

#include <cmocka.h>
#include <stdbool.h>

#include "fs.h"

#include "ssmap.h"

static int check_ssmap_equal(CMockaValueData value, CMockaValueData check_data) {

	const struct SSmap* const actual = (struct SSmap*)value.ptr;
	const struct SSmap* const expected = (struct SSmap*)check_data.ptr;

	if (ssmap_equal(actual, expected)) {
		return true;
	} else {
		fs_file_write("actual.ssmap", ssmap_str(actual), "w");
		fs_file_write("expected.ssmap", ssmap_str(expected), "w");
		cmocka_print_error("\n%s != \n%s\n",  ssmap_str(actual), ssmap_str(expected));
		return false;
	}
}

#define expect_ssmap(function, parameter, value) \
	expect_check_data(function, parameter, check_ssmap_equal, cast_ptr_to_cmocka_value(value))

#endif // EXPECT_SSMAP_H
