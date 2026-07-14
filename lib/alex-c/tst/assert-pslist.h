#ifndef ASSERT_PSLIST_H
#define ASSERT_PSLIST_H

#include <cmocka.h>

#include "fn.h"
#include "fs.h"

#include "pslist.h"

void _assert_pslist_equal(struct Pslist *a, struct Pslist *b, fn_2pred equal_val, fn_str str_val, const char * const file, const int line) {
	if (!pslist_equal(a, b, equal_val)) {
		fs_file_write("actual.pslist", pslist_str(a, str_val), "w");
		fs_file_write("expected.pslist", pslist_str(b, str_val), "w");
		cmocka_print_error("\n%s != \n%s", pslist_str(a, str_val), pslist_str(b, str_val));
		_fail(file, line);
	}
}
#define assert_pslist_equal(a, b, equal, str) _assert_pslist_equal(a, b, equal, str, __FILE__, __LINE__)

void _assert_pslist_not_equal(struct Pslist *a, struct Pslist *b, fn_2pred equal_val, fn_str str_val, const char * const file, const int line) {
	if (pslist_equal(a, b, equal_val)) {
		fs_file_write("actual.pslist", pslist_str(a, str_val), "w");
		fs_file_write("expected.pslist", pslist_str(b, str_val), "w");
		cmocka_print_error("\n%s == \n%s", pslist_str(a, str_val), pslist_str(b, str_val));
		_fail(file, line);
	}
}
#define assert_pslist_not_equal(a, b, equal, str) _assert_pslist_not_equal(a, b, equal, str, __FILE__, __LINE__)

#endif // ASSERT_PSLIST_H
