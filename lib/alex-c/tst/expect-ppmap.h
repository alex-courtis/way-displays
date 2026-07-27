#ifndef EXPECT_PPMAP_H
#define EXPECT_PPMAP_H

#include <cmocka.h>
#include <stdbool.h>

#include "fs.h"

#include "ppmap.h"

static int check_ppmap_equal_ordered(CMockaValueData value, CMockaValueData check_data) {

	const struct PPmap* const actual = (struct PPmap*)value.ptr;
	const struct PPmap* const expected = (struct PPmap*)check_data.ptr;

	if (ppmap_equal_ordered(actual, expected)) {
		return true;
	} else {
		fs_file_write("actual.ppmap", ppmap_str(actual), "w");
		fs_file_write("expected.ppmap", ppmap_str(expected), "w");
		cmocka_print_error("\n%s != \n%s\n",  ppmap_str(actual), ppmap_str(expected));
		return false;
	}
}

#define expect_ppmap(function, parameter, value) \
	expect_check_data(function, parameter, check_ppmap_equal_ordered, cast_ptr_to_cmocka_value(value))

#endif // EXPECT_PPMAP_H
