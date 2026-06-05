#ifndef ASSERT_SLIST_H
#define ASSERT_SLIST_H

#include <cmocka.h>

#include "util-file.h"

#include "fn.h"
#include "slist.h"

void _assert_slist_equal(struct SList *a, struct SList *b, fn_equal equal, fn_str str, const char * const file, const int line) {
	if (!slist_equal(a, b, equal)) {
		write_file("actual.slist", slist_str(a, str));
		write_file("expected.slist", slist_str(b, str));
		cmocka_print_error("\n%s != \n%s", slist_str(a, str), slist_str(b, str));
		_fail(file, line);
	}
}
#define assert_slist_equal(a, b, equal, str) _assert_slist_equal(a, b, equal, str, __FILE__, __LINE__)

void _assert_slist_not_equal(struct SList *a, struct SList *b, fn_equal equal, fn_str str, const char * const file, const int line) {
	if (slist_equal(a, b, equal)) {
		write_file("actual.slist", slist_str(a, str));
		write_file("expected.slist", slist_str(b, str));
		cmocka_print_error("\n%s == \n%s", slist_str(a, str), slist_str(b, str));
		_fail(file, line);
	}
}
#define assert_slist_not_equal(a, b, equal, str) _assert_slist_not_equal(a, b, equal, str, __FILE__, __LINE__)

#endif // ASSERT_SLIST_H
