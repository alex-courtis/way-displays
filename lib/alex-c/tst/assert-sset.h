#ifndef ASSERT_SSET_H
#define ASSERT_SSET_H

#include <cmocka.h>

#include "fs.h"

#include "sset.h"

void _assert_sset_equal(const struct Sset *a, const struct Sset *b, const char * const file, const int line) {
	if (!sset_equal(a, b)) {
		fs_file_write("actual.sset", sset_str(a), "w");
		fs_file_write("expected.sset", sset_str(b), "w");
		cmocka_print_error("\n%s != \n%s", sset_str(a), sset_str(b));
		_fail(file, line);
	}
}
#define assert_sset_equal(a, b) _assert_sset_equal(a, b, __FILE__, __LINE__)

void _assert_sset_not_equal(const struct Sset *a, const struct Sset *b, const char * const file, const int line) {
	if (sset_equal(a, b)) {
		fs_file_write("actual.sset", sset_str(a), "w");
		fs_file_write("expected.sset", sset_str(b), "w");
		cmocka_print_error("\n%s == \n%s", sset_str(a), sset_str(b));
		_fail(file, line);
	}
}
#define assert_sset_not_equal(a, b) _assert_sset_not_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_SSET_H
