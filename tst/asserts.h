#ifndef ASSERTS_H
#define ASSERTS_H

#include <cmocka.h>

#include "wayland-util.h"

static void _assert_wl_fixed_t_equal_double(wl_fixed_t a, double b,
	const char * const file, const int line) {

	if (a != wl_fixed_from_double(b)) {
		cmocka_print_error("%g != %g\n", wl_fixed_to_double(a), b);
		_fail(file, line);
	}
}
#define assert_wl_fixed_t_equal_double(a, b) _assert_wl_fixed_t_equal_double(a, b, __FILE__, __LINE__)

#endif // ASSERTS_H

