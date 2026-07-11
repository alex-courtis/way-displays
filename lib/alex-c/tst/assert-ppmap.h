#ifndef ASSERT_PPMAP_H
#define ASSERT_PPMAP_H

#include <cmocka.h>

#include "util-file.h"

#include "ppmap.h"

void _assert_ppmap_equal(const struct PPmap *a, const struct PPmap *b, const char * const file, const int line) {
	if (!ppmap_equal(a, b)) {
		write_file("actual.ppmap", ppmap_str(a));
		write_file("expected.ppmap", ppmap_str(b));
		cmocka_print_error("\n%s != \n%s", ppmap_str(a), ppmap_str(b));
		_fail(file, line);
	}
}
#define assert_ppmap_equal(a, b) _assert_ppmap_equal(a, b, __FILE__, __LINE__)

void _assert_ppmap_not_equal(const struct PPmap *a, const struct PPmap *b, const char * const file, const int line) {
	if (ppmap_equal(a, b)) {
		write_file("actual.ppmap", ppmap_str(a));
		write_file("expected.ppmap", ppmap_str(b));
		cmocka_print_error("\n%s == \n%s", ppmap_str(a), ppmap_str(b));
		_fail(file, line);
	}
}
#define assert_ppmap_not_equal(a, b) _assert_ppmap_not_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_PPMAP_H
