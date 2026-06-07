#ifndef ASSERT_ITABLE_H
#define ASSERT_ITABLE_H

#include <cmocka.h>

#include "util-file.h"

#include "fn.h"
#include "itable.h"

void _assert_itable_equal(const struct ITable *a, const struct ITable *b, fn_equal equal, fn_str str, const char * const file, const int line) {
	if (!itable_equal(a, b, equal)) {
		write_file("actual.itable", itable_str(a, str));
		write_file("expected.itable", itable_str(b, str));
		cmocka_print_error("\n%s != \n%s", itable_str(a, str), itable_str(b, str));
		_fail(file, line);
	}
}
#define assert_itable_equal(a, b, equal, str) _assert_itable_equal(a, b, equal, str, __FILE__, __LINE__)

void _assert_itable_not_equal(const struct ITable *a, const struct ITable *b, fn_equal equal, fn_str str, const char * const file, const int line) {
	if (itable_equal(a, b, equal)) {
		write_file("actual.itable", itable_str(a, str));
		write_file("expected.pet", itable_str(b, str));
		cmocka_print_error("\n%s == \n%s", itable_str(a, str), itable_str(b, str));
		_fail(file, line);
	}
}
#define assert_itable_not_equal(a, b, equal, str) _assert_itable_not_equal(a, b, equal, str, __FILE__, __LINE__)

#endif // ASSERT_ITABLE_H
