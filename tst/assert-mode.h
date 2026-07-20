#ifndef ASSERT_MODE_H
#define ASSERT_MODE_H

#include <cmocka.h>

#include "mode.h"

void _assert_mode_equal(const struct Mode* const a, const struct Mode* const b, const char * const file, const int line) {
	if (!mode_equal(a, b)) {
		cmocka_print_error("assert_mode_equal\n%s\n !=\n%s\n", mode_str(a), mode_str(b));
		_fail(file, line);
	}
}

#define assert_mode_equal(a, b) _assert_mode_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_MODE_H

