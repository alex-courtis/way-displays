#ifndef ASSERT_SSMAP_H
#define ASSERT_SSMAP_H

#include <cmocka.h>

#include "fs.h"

#include "ssmap.h"

void _assert_ssmap_equal(const struct SSmap *a, const struct SSmap *b, const char * const file, const int line) {
	if (!ssmap_equal(a, b)) {
		fs_file_write("actual.ssmap", ssmap_str(a), "w");
		fs_file_write("expected.ssmap", ssmap_str(b), "w");
		cmocka_print_error("\n%s != \n%s\n",  ssmap_str(a), ssmap_str(b));
		_fail(file, line);
	}
}
#define assert_ssmap_equal(a, b) _assert_ssmap_equal(a, b, __FILE__, __LINE__)

void _assert_ssmap_not_equal(const struct SSmap *a, const struct SSmap *b, const char * const file, const int line) {
	if (ssmap_equal(a, b)) {
		fs_file_write("actual.ssmap", ssmap_str(a), "w");
		fs_file_write("expected.ssmap", ssmap_str(b), "w");
		cmocka_print_error("\n%s == \n%s\n",  ssmap_str(a), ssmap_str(b));
		_fail(file, line);
	}
}
#define assert_ssmap_not_equal(a, b) _assert_ssmap_not_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_SSMAP_H
