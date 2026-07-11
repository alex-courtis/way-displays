#ifndef EXPECT_SIMAP_H
#define EXPECT_SIMAP_H

#include <cmocka.h>
#include <stdbool.h>

#include "util-file.h"

#include "simap.h"

static int check_simap_equal(CMockaValueData value, CMockaValueData check_data) {

	const struct SImap* const actual = (struct SImap*)value.ptr;
	const struct SImap* const expected = (struct SImap*)check_data.ptr;

	if (simap_equal(actual, expected)) {
		return true;
	} else {
		write_file("actual.simap", simap_str(actual));
		write_file("expected.simap", simap_str(expected));
		cmocka_print_error("\n%s != \n%s", simap_str(actual), simap_str(expected));
		return false;
	}
}

#define expect_simap(function, parameter, value) \
	expect_check_data(function, parameter, check_simap_equal, cast_ptr_to_cmocka_value(value))

#endif // EXPECT_SIMAP_H
