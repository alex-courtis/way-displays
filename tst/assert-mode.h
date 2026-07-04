#ifndef ASSERT_MODE_H
#define ASSERT_MODE_H

#include <cmocka.h>

#include "mode.h"

void _assert_mode_equal(const struct WlrMode* const a, const struct WlrMode* const b, const char * const file, const int line) {
	if (!wlr_mode_equal(a, b)) {
		cmocka_print_error("assert_mode_equal\n%s\n !=\n%s\n", wlr_mode_str(a), wlr_mode_str(b));
		_fail(file, line);
	}
}

#define assert_mode_equal(a, b) _assert_mode_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_MODE_H

