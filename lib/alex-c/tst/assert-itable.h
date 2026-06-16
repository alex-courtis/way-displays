#ifndef ASSERT_ITABLE_H
#define ASSERT_ITABLE_H

#include <cmocka.h>

#include "util-file.h"

#include "itable.h"

void _assert_itable_equal(const struct ITable *a, const struct ITable *b, const char * const file, const int line) {
	if (!itable_equal(a, b)) {
		write_file("actual.itable", itable_str(a));
		write_file("expected.itable", itable_str(b));
		cmocka_print_error("\n%s != \n%s", itable_str(a), itable_str(b));
		_fail(file, line);
	}
}
#define assert_itable_equal(a, b) _assert_itable_equal(a, b, __FILE__, __LINE__)

void _assert_itable_not_equal(const struct ITable *a, const struct ITable *b, const char * const file, const int line) {
	if (itable_equal(a, b)) {
		write_file("actual.itable", itable_str(a));
		write_file("expected.itable", itable_str(b));
		cmocka_print_error("\n%s == \n%s", itable_str(a), itable_str(b));
		_fail(file, line);
	}
}
#define assert_itable_not_equal(a, b) _assert_itable_not_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_ITABLE_H
