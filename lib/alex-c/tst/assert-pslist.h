#ifndef ASSERT_PSLIST_H
#define ASSERT_PSLIST_H

#include <cmocka.h>

#include "util-file.h"

#include "fn.h"
#include "pslist.h"

void _assert_pslist_equal(struct Pslist *a, struct Pslist *b, fn_2pred equal_val, fn_str str_val, const char * const file, const int line) {
	if (!pslist_equal(a, b, equal_val)) {
		write_file("actual.pslist", pslist_str(a, str_val));
		write_file("expected.pslist", pslist_str(b, str_val));
		cmocka_print_error("\n%s != \n%s", pslist_str(a, str_val), pslist_str(b, str_val));
		_fail(file, line);
	}
}
#define assert_pslist_equal(a, b, equal, str) _assert_pslist_equal(a, b, equal, str, __FILE__, __LINE__)

void _assert_pslist_not_equal(struct Pslist *a, struct Pslist *b, fn_2pred equal_val, fn_str str_val, const char * const file, const int line) {
	if (pslist_equal(a, b, equal_val)) {
		write_file("actual.pslist", pslist_str(a, str_val));
		write_file("expected.pslist", pslist_str(b, str_val));
		cmocka_print_error("\n%s == \n%s", pslist_str(a, str_val), pslist_str(b, str_val));
		_fail(file, line);
	}
}
#define assert_pslist_not_equal(a, b, equal, str) _assert_pslist_not_equal(a, b, equal, str, __FILE__, __LINE__)

#endif // ASSERT_PSLIST_H
