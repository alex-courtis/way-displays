#ifndef ASSERT_SSET_H
#define ASSERT_SSET_H

#include <cmocka.h>

#include "util-file.h"

#include "sset.h"

void _assert_sset_equal(const struct SSet *a, const struct SSet *b, const char * const file, const int line) {
	if (!sset_equal(a, b)) {
		write_file("actual.sset", sset_str(a));
		write_file("expected.sset", sset_str(b));
		cmocka_print_error("\n%s != \n%s", sset_str(a), sset_str(b));
		_fail(file, line);
	}
}
#define assert_sset_equal(a, b) _assert_sset_equal(a, b, __FILE__, __LINE__)

void _assert_sset_not_equal(const struct SSet *a, const struct SSet *b, const char * const file, const int line) {
	if (sset_equal(a, b)) {
		write_file("actual.sset", sset_str(a));
		write_file("expected.sset", sset_str(b));
		cmocka_print_error("\n%s == \n%s", sset_str(a), sset_str(b));
		_fail(file, line);
	}
}
#define assert_sset_not_equal(a, b) _assert_sset_not_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_SSET_H
