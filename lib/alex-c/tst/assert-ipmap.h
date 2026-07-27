#ifndef ASSERT_IPMAP_H
#define ASSERT_IPMAP_H

#include <cmocka.h>

#include "fs.h"

#include "ipmap.h"

void _assert_ipmap_equal(const struct IPmap *a, const struct IPmap *b, const char * const file, const int line) {
	if (!ipmap_equal(a, b)) {
		fs_file_write("actual.ipmap", ipmap_str(a), "w");
		fs_file_write("expected.ipmap", ipmap_str(b), "w");
		cmocka_print_error("\n%s != \n%s\n",  ipmap_str(a), ipmap_str(b));
		_fail(file, line);
	}
}
#define assert_ipmap_equal(a, b) _assert_ipmap_equal(a, b, __FILE__, __LINE__)

void _assert_ipmap_not_equal(const struct IPmap *a, const struct IPmap *b, const char * const file, const int line) {
	if (ipmap_equal(a, b)) {
		fs_file_write("actual.ipmap", ipmap_str(a), "w");
		fs_file_write("expected.ipmap", ipmap_str(b), "w");
		cmocka_print_error("\n%s == \n%s\n",  ipmap_str(a), ipmap_str(b));
		_fail(file, line);
	}
}
#define assert_ipmap_not_equal(a, b) _assert_ipmap_not_equal(a, b, __FILE__, __LINE__)

void _assert_ipmap_equal_ordered(const struct IPmap *a, const struct IPmap *b, const char * const file, const int line) {
	if (!ipmap_equal_ordered(a, b)) {
		fs_file_write("actual.ipmap", ipmap_str(a), "w");
		fs_file_write("expected.ipmap", ipmap_str(b), "w");
		cmocka_print_error("\n%s != \n%s\n",  ipmap_str(a), ipmap_str(b));
		_fail(file, line);
	}
}
#define assert_ipmap_equal_ordered(a, b) _assert_ipmap_equal_ordered(a, b, __FILE__, __LINE__)

void _assert_ipmap_not_equal_ordered(const struct IPmap *a, const struct IPmap *b, const char * const file, const int line) {
	if (ipmap_equal_ordered(a, b)) {
		fs_file_write("actual.ipmap", ipmap_str(a), "w");
		fs_file_write("expected.ipmap", ipmap_str(b), "w");
		cmocka_print_error("\n%s == \n%s\n",  ipmap_str(a), ipmap_str(b));
		_fail(file, line);
	}
}
#define assert_ipmap_not_equal_ordered(a, b) _assert_ipmap_not_equal_ordered(a, b, __FILE__, __LINE__)

#endif // ASSERT_IPMAP_H
