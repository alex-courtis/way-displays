#ifndef ASSERT_STABLE_H
#define ASSERT_STABLE_H

#include <cmocka.h>

#include "util-file.h"

#include "fn.h"
#include "stable.h"

void _assert_stable_equal(const struct STable *a, const struct STable *b, fn_equal equal, fn_str str, const char * const file, const int line) {
	if (!stable_equal(a, b, equal)) {
		write_file("actual.stable", stable_str(a, str));
		write_file("expected.stable", stable_str(b, str));
		cmocka_print_error("\n%s != \n%s", stable_str(a, str), stable_str(b, str));
		_fail(file, line);
	}
}
#define assert_stable_equal(a, b, equal, str) _assert_stable_equal(a, b, equal, str, __FILE__, __LINE__)

void _assert_stable_not_equal(const struct STable *a, const struct STable *b, fn_equal equal, fn_str str, const char * const file, const int line) {
	if (stable_equal(a, b, equal)) {
		write_file("actual.stable", stable_str(a, str));
		write_file("expected.pet", stable_str(b, str));
		cmocka_print_error("\n%s == \n%s", stable_str(a, str), stable_str(b, str));
		_fail(file, line);
	}
}
#define assert_stable_not_equal(a, b, equal, str) _assert_stable_not_equal(a, b, equal, str, __FILE__, __LINE__)

#endif // ASSERT_STABLE_H
