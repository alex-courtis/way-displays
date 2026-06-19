#ifndef ASSERT_PMAP_H
#define ASSERT_PMAP_H

#include <cmocka.h>

#include "util-file.h"

#include "pmap.h"

void _assert_pmap_equal(const struct PMap *a, const struct PMap *b, const char * const file, const int line) {
	if (!pmap_equal(a, b)) {
		write_file("actual.pmap", pmap_str(a));
		write_file("expected.pmap", pmap_str(b));
		cmocka_print_error("\n%s != \n%s", pmap_str(a), pmap_str(b));
		_fail(file, line);
	}
}
#define assert_pmap_equal(a, b) _assert_pmap_equal(a, b, __FILE__, __LINE__)

void _assert_pmap_not_equal(const struct PMap *a, const struct PMap *b, const char * const file, const int line) {
	if (pmap_equal(a, b)) {
		write_file("actual.pmap", pmap_str(a));
		write_file("expected.pmap", pmap_str(b));
		cmocka_print_error("\n%s == \n%s", pmap_str(a), pmap_str(b));
		_fail(file, line);
	}
}
#define assert_pmap_not_equal(a, b) _assert_pmap_not_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_PMAP_H
