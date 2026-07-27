#ifndef ASSERT_PLIST_H
#define ASSERT_PLIST_H

#include <cmocka.h>

#include "fs.h"

#include "plist.h"

void _assert_plist_equal(const struct Plist *a, const struct Plist *b, const char * const file, const int line) {
	if (!plist_equal(a, b)) {
		fs_file_write("actual.pset", plist_str(a), "w");
		fs_file_write("expected.pset", plist_str(b), "w");
		cmocka_print_error("\n%s != \n%s\n",  plist_str(a), plist_str(b));
		_fail(file, line);
	}
}
#define assert_plist_equal(a, b) _assert_plist_equal(a, b, __FILE__, __LINE__)

void _assert_plist_not_equal(const struct Plist *a, const struct Plist *b, const char * const file, const int line) {
	if (plist_equal(a, b)) {
		fs_file_write("actual.pset", plist_str(a), "w");
		fs_file_write("expected.pset", plist_str(b), "w");
		cmocka_print_error("\n%s == \n%s\n",  plist_str(a), plist_str(b));
		_fail(file, line);
	}
}
#define assert_plist_not_equal(a, b) _assert_plist_not_equal(a, b, __FILE__, __LINE__)

void _assert_plist_equal_ordered(const struct Plist *a, const struct Plist *b, const char * const file, const int line) {
	if (!plist_equal_ordered(a, b)) {
		fs_file_write("actual.pset", plist_str(a), "w");
		fs_file_write("expected.pset", plist_str(b), "w");
		cmocka_print_error("\n%s != \n%s\n",  plist_str(a), plist_str(b));
		_fail(file, line);
	}
}
#define assert_plist_equal_ordered(a, b) _assert_plist_equal_ordered(a, b, __FILE__, __LINE__)

void _assert_plist_not_equal_ordered(const struct Plist *a, const struct Plist *b, const char * const file, const int line) {
	if (plist_equal_ordered(a, b)) {
		fs_file_write("actual.pset", plist_str(a), "w");
		fs_file_write("expected.pset", plist_str(b), "w");
		cmocka_print_error("\n%s == \n%s\n",  plist_str(a), plist_str(b));
		_fail(file, line);
	}
}
#define assert_plist_not_equal_ordered(a, b) _assert_plist_not_equal_ordered(a, b, __FILE__, __LINE__)

#endif // ASSERT_PLIST_H
