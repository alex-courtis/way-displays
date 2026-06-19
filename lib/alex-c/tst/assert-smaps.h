#ifndef ASSERT_SMAPS_H
#define ASSERT_SMAPS_H

#include <cmocka.h>

#include "util-file.h"

#include "smaps.h"

void _assert_smaps_equal(const struct SMapS *a, const struct SMapS *b, const char * const file, const int line) {
	if (!smaps_equal(a, b)) {
		write_file("actual.smaps", smaps_str(a));
		write_file("expected.smaps", smaps_str(b));
		cmocka_print_error("\n%s != \n%s", smaps_str(a), smaps_str(b));
		_fail(file, line);
	}
}
#define assert_smaps_equal(a, b) _assert_smaps_equal(a, b, __FILE__, __LINE__)

void _assert_smaps_not_equal(const struct SMapS *a, const struct SMapS *b, const char * const file, const int line) {
	if (smaps_equal(a, b)) {
		write_file("actual.smaps", smaps_str(a));
		write_file("expected.smaps", smaps_str(b));
		cmocka_print_error("\n%s == \n%s", smaps_str(a), smaps_str(b));
		_fail(file, line);
	}
}
#define assert_smaps_not_equal(a, b) _assert_smaps_not_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_SMAPS_H
