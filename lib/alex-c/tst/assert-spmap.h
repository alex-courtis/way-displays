#ifndef ASSERT_SPMAP_H
#define ASSERT_SPMAP_H

#include <cmocka.h>

#include "fs.h"

#include "spmap.h"

void _assert_spmap_equal(const struct SPmap *a, const struct SPmap *b, const char * const file, const int line) {
	if (!spmap_equal(a, b)) {
		fs_file_write("actual.spmap", spmap_str(a), "w");
		fs_file_write("expected.spmap", spmap_str(b), "w");
		cmocka_print_error("\n%s != \n%s\n",  spmap_str(a), spmap_str(b));
		_fail(file, line);
	}
}
#define assert_spmap_equal(a, b) _assert_spmap_equal(a, b, __FILE__, __LINE__)

void _assert_spmap_not_equal(const struct SPmap *a, const struct SPmap *b, const char * const file, const int line) {
	if (spmap_equal(a, b)) {
		fs_file_write("actual.spmap", spmap_str(a), "w");
		fs_file_write("expected.spmap", spmap_str(b), "w");
		cmocka_print_error("\n%s == \n%s\n",  spmap_str(a), spmap_str(b));
		_fail(file, line);
	}
}
#define assert_spmap_not_equal(a, b) _assert_spmap_not_equal(a, b, __FILE__, __LINE__)

void _assert_spmap_equal_ordered(const struct SPmap *a, const struct SPmap *b, const char * const file, const int line) {
	if (!spmap_equal_ordered(a, b)) {
		fs_file_write("actual.spmap", spmap_str(a), "w");
		fs_file_write("expected.spmap", spmap_str(b), "w");
		cmocka_print_error("\n%s != \n%s\n",  spmap_str(a), spmap_str(b));
		_fail(file, line);
	}
}
#define assert_spmap_equal_ordered(a, b) _assert_spmap_equal_ordered(a, b, __FILE__, __LINE__)

void _assert_spmap_not_equal_ordered(const struct SPmap *a, const struct SPmap *b, const char * const file, const int line) {
	if (spmap_equal_ordered(a, b)) {
		fs_file_write("actual.spmap", spmap_str(a), "w");
		fs_file_write("expected.spmap", spmap_str(b), "w");
		cmocka_print_error("\n%s == \n%s\n",  spmap_str(a), spmap_str(b));
		_fail(file, line);
	}
}
#define assert_spmap_not_equal_ordered(a, b) _assert_spmap_not_equal_ordered(a, b, __FILE__, __LINE__)

#endif // ASSERT_SPMAP_H
