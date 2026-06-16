#ifndef ASSERT_STABLE_H
#define ASSERT_STABLE_H

#include <cmocka.h>
#include <stddef.h>

#include "util-file.h"

#include "stable.h"

void _assert_stable_equal(const struct STable *a, const struct STable *b, const char * const file, const int line) {
	if (!stable_equal(a, b)) {
		write_file("actual.stable", stable_str(a, NULL));
		write_file("expected.stable", stable_str(b, NULL));
		cmocka_print_error("\n%s != \n%s", stable_str(a, NULL), stable_str(b, NULL));
		_fail(file, line);
	}
}
#define assert_stable_equal(a, b) _assert_stable_equal(a, b, __FILE__, __LINE__)

void _assert_stable_not_equal(const struct STable *a, const struct STable *b, const char * const file, const int line) {
	if (stable_equal(a, b)) {
		write_file("actual.stable", stable_str(a, NULL));
		write_file("expected.stable", stable_str(b, NULL));
		cmocka_print_error("\n%s == \n%s", stable_str(a, NULL), stable_str(b, NULL));
		_fail(file, line);
	}
}
#define assert_stable_not_equal(a, b) _assert_stable_not_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_STABLE_H
