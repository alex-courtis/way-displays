#ifndef EXPECT_SMAPS_H
#define EXPECT_SMAPS_H

#include <cmocka.h>
#include <stdbool.h>

#include "util-file.h"

#include "smaps.h"

static int check_smaps_equal(CMockaValueData value, CMockaValueData check_data) {

	const struct SMapS* const actual = (struct SMapS*)value.ptr;
	const struct SMapS* const expected = (struct SMapS*)check_data.ptr;

	if (smaps_equal(actual, expected)) {
		return true;
	} else {
		write_file("actual.smaps", smaps_str(actual));
		write_file("expected.smaps", smaps_str(expected));
		cmocka_print_error("\n%s != \n%s", smaps_str(actual), smaps_str(expected));
		return false;
	}
}

#define expect_smaps(function, parameter, value) \
	expect_check_data(function, parameter, check_smaps_equal, cast_ptr_to_cmocka_value(value))

#endif // EXPECT_SMAPS_H
