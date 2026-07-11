#ifndef ASSERT_SIMAP_H
#define ASSERT_SIMAP_H

#include <cmocka.h>

#include "util-file.h"

#include "simap.h"

void _assert_simap_equal(const struct SImap *a, const struct SImap *b, const char * const file, const int line) {
	if (!simap_equal(a, b)) {
		write_file("actual.simap", simap_str(a));
		write_file("expected.simap", simap_str(b));
		cmocka_print_error("\n%s != \n%s", simap_str(a), simap_str(b));
		_fail(file, line);
	}
}
#define assert_simap_equal(a, b) _assert_simap_equal(a, b, __FILE__, __LINE__)

void _assert_simap_not_equal(const struct SImap *a, const struct SImap *b, const char * const file, const int line) {
	if (simap_equal(a, b)) {
		write_file("actual.simap", simap_str(a));
		write_file("expected.simap", simap_str(b));
		cmocka_print_error("\n%s == \n%s", simap_str(a), simap_str(b));
		_fail(file, line);
	}
}
#define assert_simap_not_equal(a, b) _assert_simap_not_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_SIMAP_H
