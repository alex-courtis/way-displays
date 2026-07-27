#ifndef ASSERT_SLIST_H
#define ASSERT_SLIST_H

#include <cmocka.h>

#include "fs.h"

#include "slist.h"

void _assert_slist_equal(const struct Slist *a, const struct Slist *b, const char * const file, const int line) {
	if (!slist_equal(a, b)) {
		fs_file_write("actual.pset", slist_str(a), "w");
		fs_file_write("expected.pset", slist_str(b), "w");
		cmocka_print_error("\n%s != \n%s\n",  slist_str(a), slist_str(b));
		_fail(file, line);
	}
}
#define assert_slist_equal(a, b) _assert_slist_equal(a, b, __FILE__, __LINE__)

void _assert_slist_not_equal(const struct Slist *a, const struct Slist *b, const char * const file, const int line) {
	if (slist_equal(a, b)) {
		fs_file_write("actual.pset", slist_str(a), "w");
		fs_file_write("expected.pset", slist_str(b), "w");
		cmocka_print_error("\n%s == \n%s\n",  slist_str(a), slist_str(b));
		_fail(file, line);
	}
}
#define assert_slist_not_equal(a, b) _assert_slist_not_equal(a, b, __FILE__, __LINE__)

void _assert_slist_equal_ordered(const struct Slist *a, const struct Slist *b, const char * const file, const int line) {
	if (!slist_equal_ordered(a, b)) {
		fs_file_write("actual.pset", slist_str(a), "w");
		fs_file_write("expected.pset", slist_str(b), "w");
		cmocka_print_error("\n%s != \n%s\n",  slist_str(a), slist_str(b));
		_fail(file, line);
	}
}
#define assert_slist_equal_ordered(a, b) _assert_slist_equal_ordered(a, b, __FILE__, __LINE__)

void _assert_slist_not_equal_ordered(const struct Slist *a, const struct Slist *b, const char * const file, const int line) {
	if (slist_equal_ordered(a, b)) {
		fs_file_write("actual.pset", slist_str(a), "w");
		fs_file_write("expected.pset", slist_str(b), "w");
		cmocka_print_error("\n%s == \n%s\n",  slist_str(a), slist_str(b));
		_fail(file, line);
	}
}
#define assert_slist_not_equal_ordered(a, b) _assert_slist_not_equal_ordered(a, b, __FILE__, __LINE__)

#endif // ASSERT_SLIST_H
