#ifndef ASSERT_SLIST_H
#define ASSERT_SLIST_H

#include <cmocka.h>

#include "util-file.h"

#include "fn.h"
#include "slist.h"

void _assert_slist_equal(struct SList *a, struct SList *b, fn_2pred equal_val, fn_str str_val, const char * const file, const int line) {
	if (!slist_equal(a, b, equal_val)) {
		write_file("actual.slist", slist_str(a, str_val));
		write_file("expected.slist", slist_str(b, str_val));
		cmocka_print_error("\n%s != \n%s", slist_str(a, str_val), slist_str(b, str_val));
		_fail(file, line);
	}
}
#define assert_slist_equal(a, b, equal, str) _assert_slist_equal(a, b, equal, str, __FILE__, __LINE__)

void _assert_slist_not_equal(struct SList *a, struct SList *b, fn_2pred equal_val, fn_str str_val, const char * const file, const int line) {
	if (slist_equal(a, b, equal_val)) {
		write_file("actual.slist", slist_str(a, str_val));
		write_file("expected.slist", slist_str(b, str_val));
		cmocka_print_error("\n%s == \n%s", slist_str(a, str_val), slist_str(b, str_val));
		_fail(file, line);
	}
}
#define assert_slist_not_equal(a, b, equal, str) _assert_slist_not_equal(a, b, equal, str, __FILE__, __LINE__)

#endif // ASSERT_SLIST_H
