#ifndef ASSERT_SPMAP_H
#define ASSERT_SPMAP_H

#include <cmocka.h>

#include "util-file.h"

#include "spmap.h"

void _assert_spmap_equal(const struct SPmap *a, const struct SPmap *b, const char * const file, const int line) {
	if (!spmap_equal(a, b)) {
		write_file("actual.spmap", spmap_str(a));
		write_file("expected.spmap", spmap_str(b));
		cmocka_print_error("\n%s != \n%s", spmap_str(a), spmap_str(b));
		_fail(file, line);
	}
}
#define assert_spmap_equal(a, b) _assert_spmap_equal(a, b, __FILE__, __LINE__)

void _assert_spmap_not_equal(const struct SPmap *a, const struct SPmap *b, const char * const file, const int line) {
	if (spmap_equal(a, b)) {
		write_file("actual.spmap", spmap_str(a));
		write_file("expected.spmap", spmap_str(b));
		cmocka_print_error("\n%s == \n%s", spmap_str(a), spmap_str(b));
		_fail(file, line);
	}
}
#define assert_spmap_not_equal(a, b) _assert_spmap_not_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_SPMAP_H
