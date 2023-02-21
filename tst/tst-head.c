// IWYU pragma: no_include <cmocka.h>
#include "tst.h" // IWYU pragma: keep

#include <stddef.h>

#include "layout.h"
#include "list.h"

int setup(void **state) {
	return 0;
}

int teardown(void **state) {
	return 0;
}

void blargh(void **state) {
	assert_int_equal(1, 2);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(blargh),
	};

	return cmocka_run_group_tests(tests, setup, teardown);
}

