#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <cmocka.h>

#include "test-add.c"
#include "test-calc-auto-scale.c"
#include "test-calc-ltr-heads.c"
#include "test-calc-optimal-mode.c"
#include "test-calc-order-heads.c"
#include "test-laptop.c"

static int setup(void **state) {
	return 0;
}

static int teardown(void **state) {
	return 0;
}

int main(void) {
	const struct CMUnitTest tests[] = {
		add_tests,
		calc_auto_scale_tests,
		calc_ltr_heads_tests,
		calc_optimal_mode_tests,
		calc_order_heads_tests,
		laptop_tests,
	};
	return cmocka_run_group_tests(tests, setup, teardown);
}

