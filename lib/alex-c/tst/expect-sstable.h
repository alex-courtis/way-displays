#ifndef EXPECT_SSTABLE_H
#define EXPECT_SSTABLE_H

#include <cmocka.h>
#include <stdbool.h>

#include "util-file.h"

#include "sstable.h"

static int check_sstable_equal_strcmp(CMockaValueData value, CMockaValueData check_data) {

	const struct SSTable* const actual = (struct SSTable*)value.ptr;
	const struct SSTable* const expected = (struct SSTable*)check_data.ptr;

	if (sstable_equal(actual, expected)) {
		return true;
	} else {
		write_file("actual.sstable", sstable_str(actual));
		write_file("expected.sstable", sstable_str(expected));
		cmocka_print_error("\n%s != \n%s", sstable_str(actual), sstable_str(expected));
		return false;
	}
}

#define expect_sstable_strcmp(function, parameter, value) \
	expect_check_data(function, parameter, check_sstable_equal_strcmp, cast_ptr_to_cmocka_value(value))

#endif // EXPECT_SSTABLE_H
