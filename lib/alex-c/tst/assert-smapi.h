#ifndef ASSERT_SMAPI_H
#define ASSERT_SMAPI_H

#include <cmocka.h>

#include "util-file.h"

#include "smapi.h"

void _assert_smapi_equal(const struct SMapI *a, const struct SMapI *b, const char * const file, const int line) {
	if (!smapi_equal(a, b)) {
		write_file("actual.smapi", smapi_str(a));
		write_file("expected.smapi", smapi_str(b));
		cmocka_print_error("\n%s != \n%s", smapi_str(a), smapi_str(b));
		_fail(file, line);
	}
}
#define assert_smapi_equal(a, b) _assert_smapi_equal(a, b, __FILE__, __LINE__)

void _assert_smapi_not_equal(const struct SMapI *a, const struct SMapI *b, const char * const file, const int line) {
	if (smapi_equal(a, b)) {
		write_file("actual.smapi", smapi_str(a));
		write_file("expected.smapi", smapi_str(b));
		cmocka_print_error("\n%s == \n%s", smapi_str(a), smapi_str(b));
		_fail(file, line);
	}
}
#define assert_smapi_not_equal(a, b) _assert_smapi_not_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_SMAPI_H
