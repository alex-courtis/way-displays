#ifndef ASSERT_IPMAP_H
#define ASSERT_IPMAP_H

#include <cmocka.h>

#include "util-file.h"

#include "ipmap.h"

void _assert_ipmap_equal(const struct IPmap *a, const struct IPmap *b, const char * const file, const int line) {
	if (!ipmap_equal(a, b)) {
		write_file("actual.ipmap", ipmap_str(a));
		write_file("expected.ipmap", ipmap_str(b));
		cmocka_print_error("\n%s != \n%s", ipmap_str(a), ipmap_str(b));
		_fail(file, line);
	}
}
#define assert_ipmap_equal(a, b) _assert_ipmap_equal(a, b, __FILE__, __LINE__)

void _assert_ipmap_not_equal(const struct IPmap *a, const struct IPmap *b, const char * const file, const int line) {
	if (ipmap_equal(a, b)) {
		write_file("actual.ipmap", ipmap_str(a));
		write_file("expected.ipmap", ipmap_str(b));
		cmocka_print_error("\n%s == \n%s", ipmap_str(a), ipmap_str(b));
		_fail(file, line);
	}
}
#define assert_ipmap_not_equal(a, b) _assert_ipmap_not_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_IPMAP_H
