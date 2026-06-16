#ifndef ASSERT_PTABLE_H
#define ASSERT_PTABLE_H

#include <cmocka.h>

#include "util-file.h"

#include "ptable.h"

void _assert_ptable_equal(const struct PTable *a, const struct PTable *b, const char * const file, const int line) {
	if (!ptable_equal(a, b)) {
		write_file("actual.ptable", ptable_str(a));
		write_file("expected.ptable", ptable_str(b));
		cmocka_print_error("\n%s != \n%s", ptable_str(a), ptable_str(b));
		_fail(file, line);
	}
}
#define assert_ptable_equal(a, b) _assert_ptable_equal(a, b, __FILE__, __LINE__)

void _assert_ptable_not_equal(const struct PTable *a, const struct PTable *b, const char * const file, const int line) {
	if (ptable_equal(a, b)) {
		write_file("actual.ptable", ptable_str(a));
		write_file("expected.ptable", ptable_str(b));
		cmocka_print_error("\n%s == \n%s", ptable_str(a), ptable_str(b));
		_fail(file, line);
	}
}
#define assert_ptable_not_equal(a, b) _assert_ptable_not_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_PTABLE_H
