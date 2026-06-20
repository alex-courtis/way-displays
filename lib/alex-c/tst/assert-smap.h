#ifndef ASSERT_SMAP_H
#define ASSERT_SMAP_H

#include <cmocka.h>

#include "util-file.h"

#include "smap.h"

void _assert_smap_equal(const struct SMap *a, const struct SMap *b, const char * const file, const int line) {
	if (!smap_equal(a, b)) {
		write_file("actual.smap", smap_str(a));
		write_file("expected.smap", smap_str(b));
		cmocka_print_error("\n%s != \n%s", smap_str(a), smap_str(b));
		_fail(file, line);
	}
}
#define assert_smap_equal(a, b) _assert_smap_equal(a, b, __FILE__, __LINE__)

void _assert_smap_not_equal(const struct SMap *a, const struct SMap *b, const char * const file, const int line) {
	if (smap_equal(a, b)) {
		write_file("actual.smap", smap_str(a));
		write_file("expected.smap", smap_str(b));
		cmocka_print_error("\n%s == \n%s", smap_str(a), smap_str(b));
		_fail(file, line);
	}
}
#define assert_smap_not_equal(a, b) _assert_smap_not_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_SMAP_H
