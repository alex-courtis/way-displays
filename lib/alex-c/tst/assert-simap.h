#ifndef ASSERT_SIMAP_H
#define ASSERT_SIMAP_H

#include <cmocka.h>

#include "fs.h"

#include "simap.h"

void _assert_simap_equal(const struct SImap *a, const struct SImap *b, const char * const file, const int line) {
	if (!simap_equal(a, b)) {
		fs_file_write("actual.simap", simap_str(a), "w");
		fs_file_write("expected.simap", simap_str(b), "w");
		cmocka_print_error("\n%s != \n%s\n",  simap_str(a), simap_str(b));
		_fail(file, line);
	}
}
#define assert_simap_equal(a, b) _assert_simap_equal(a, b, __FILE__, __LINE__)

void _assert_simap_not_equal(const struct SImap *a, const struct SImap *b, const char * const file, const int line) {
	if (simap_equal(a, b)) {
		fs_file_write("actual.simap", simap_str(a), "w");
		fs_file_write("expected.simap", simap_str(b), "w");
		cmocka_print_error("\n%s == \n%s\n",  simap_str(a), simap_str(b));
		_fail(file, line);
	}
}
#define assert_simap_not_equal(a, b) _assert_simap_not_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_SIMAP_H
