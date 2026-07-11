#ifndef ASSERT_SSMAP_H
#define ASSERT_SSMAP_H

#include <cmocka.h>

#include "util-file.h"

#include "ssmap.h"

void _assert_ssmap_equal(const struct SSmap *a, const struct SSmap *b, const char * const file, const int line) {
	if (!ssmap_equal(a, b)) {
		write_file("actual.ssmap", ssmap_str(a));
		write_file("expected.ssmap", ssmap_str(b));
		cmocka_print_error("\n%s != \n%s", ssmap_str(a), ssmap_str(b));
		_fail(file, line);
	}
}
#define assert_ssmap_equal(a, b) _assert_ssmap_equal(a, b, __FILE__, __LINE__)

void _assert_ssmap_not_equal(const struct SSmap *a, const struct SSmap *b, const char * const file, const int line) {
	if (ssmap_equal(a, b)) {
		write_file("actual.ssmap", ssmap_str(a));
		write_file("expected.ssmap", ssmap_str(b));
		cmocka_print_error("\n%s == \n%s", ssmap_str(a), ssmap_str(b));
		_fail(file, line);
	}
}
#define assert_ssmap_not_equal(a, b) _assert_ssmap_not_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_SSMAP_H
