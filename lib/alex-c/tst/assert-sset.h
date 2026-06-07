#ifndef ASSERT_SSET_H
#define ASSERT_SSET_H

#include <cmocka.h>

#include "util-file.h"

#include "sset.h"

void _assert_sset_equal(const struct SSet *a, const struct SSet *b, const char * const file, const int line) {
	if (!sset_equal(a, b)) {
		char *a_str = sset_str(a);
		char *b_str = sset_str(b);
		write_file("actual.sset", a_str);
		write_file("expected.sset", b_str);
		cmocka_print_error("\n%s != \n%s", a_str, b_str);
		_fail(file, line);
	}
}
#define assert_sset_equal(a, b) _assert_sset_equal(a, b, __FILE__, __LINE__)

void _assert_sset_not_equal(const struct SSet *a, const struct SSet *b, const char * const file, const int line) {
	if (sset_equal(a, b)) {
		char *a_str = sset_str(a);
		char *b_str = sset_str(b);
		write_file("actual.sset", a_str);
		write_file("expected.sset", b_str);
		cmocka_print_error("\n%s == \n%s", a_str, b_str);
		_fail(file, line);
	}
}
#define assert_sset_not_equal(a, b) _assert_sset_not_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_SSET_H
