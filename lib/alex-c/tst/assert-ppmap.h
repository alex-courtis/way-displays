#ifndef ASSERT_PPMAP_H
#define ASSERT_PPMAP_H

#include <cmocka.h>

#include "fs.h"

#include "ppmap.h"

void _assert_ppmap_equal(const struct PPmap *a, const struct PPmap *b, const char * const file, const int line) {
	if (!ppmap_equal(a, b)) {
		fs_file_write("actual.ppmap", ppmap_str(a), "w");
		fs_file_write("expected.ppmap", ppmap_str(b), "w");
		cmocka_print_error("\n%s != \n%s\n",  ppmap_str(a), ppmap_str(b));
		_fail(file, line);
	}
}
#define assert_ppmap_equal(a, b) _assert_ppmap_equal(a, b, __FILE__, __LINE__)

void _assert_ppmap_not_equal(const struct PPmap *a, const struct PPmap *b, const char * const file, const int line) {
	if (ppmap_equal(a, b)) {
		fs_file_write("actual.ppmap", ppmap_str(a), "w");
		fs_file_write("expected.ppmap", ppmap_str(b), "w");
		cmocka_print_error("\n%s == \n%s\n",  ppmap_str(a), ppmap_str(b));
		_fail(file, line);
	}
}
#define assert_ppmap_not_equal(a, b) _assert_ppmap_not_equal(a, b, __FILE__, __LINE__)

void _assert_ppmap_equal_ordered(const struct PPmap *a, const struct PPmap *b, const char * const file, const int line) {
	if (!ppmap_equal_ordered(a, b)) {
		fs_file_write("actual.ppmap", ppmap_str(a), "w");
		fs_file_write("expected.ppmap", ppmap_str(b), "w");
		cmocka_print_error("\n%s != \n%s\n",  ppmap_str(a), ppmap_str(b));
		_fail(file, line);
	}
}
#define assert_ppmap_equal_ordered(a, b) _assert_ppmap_equal_ordered(a, b, __FILE__, __LINE__)

void _assert_ppmap_not_equal_ordered(const struct PPmap *a, const struct PPmap *b, const char * const file, const int line) {
	if (ppmap_equal_ordered(a, b)) {
		fs_file_write("actual.ppmap", ppmap_str(a), "w");
		fs_file_write("expected.ppmap", ppmap_str(b), "w");
		cmocka_print_error("\n%s == \n%s\n",  ppmap_str(a), ppmap_str(b));
		_fail(file, line);
	}
}
#define assert_ppmap_not_equal_ordered(a, b) _assert_ppmap_not_equal_ordered(a, b, __FILE__, __LINE__)

#endif // ASSERT_PPMAP_H
