#ifndef ASSERT_PSET_H
#define ASSERT_PSET_H

#include <cmocka.h>

#include "util-file.h"

#include "pset.h"

void _assert_pset_equal(const struct PSet *a, const struct PSet *b, const char * const file, const int line) {
	if (!pset_equal(a, b)) {
		write_file("actual.pset", pset_str(a));
		write_file("expected.pset", pset_str(b));
		cmocka_print_error("\n%s != \n%s", pset_str(a), pset_str(b));
		_fail(file, line);
	}
}
#define assert_pset_equal(a, b) _assert_pset_equal(a, b, __FILE__, __LINE__)

void _assert_pset_not_equal(const struct PSet *a, const struct PSet *b, const char * const file, const int line) {
	if (pset_equal(a, b)) {
		write_file("actual.pset", pset_str(a));
		write_file("expected.pset", pset_str(b));
		cmocka_print_error("\n%s == \n%s", pset_str(a), pset_str(b));
		_fail(file, line);
	}
}
#define assert_pset_not_equal(a, b) _assert_pset_not_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_PSET_H
