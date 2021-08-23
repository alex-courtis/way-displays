#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <cmocka.h>
#include <wayland-util.h>

#include "add.h"
#include "util.h"

int __real_second(int second);

int
__wrap_first(int first) {
	return 1;
}

int
__wrap_second(int second) {
	return __real_second(second);
}

static void
test_add(void **state) {
	assert_int_equal(add(3, 5), 6);
}

static void
test_optimal_mode_preferred() {

	struct wl_list modes;
	wl_list_init(&modes);

	struct Mode *mode1 = calloc(1, sizeof(*mode1));
	mode1->preferred = true;
	wl_list_insert(&modes, &mode1->link);

	struct Mode *mode2 = calloc(1, sizeof(*mode2));
	wl_list_insert(&modes, &mode2->link);


	struct Mode *mode = optimal_mode(&modes);
	assert_ptr_equal(mode, mode1);


	// TODO there must be a better way to free the list
	struct Mode *mode_removing, *mode_tmp;
	wl_list_for_each_safe(mode_removing, mode_tmp, &modes, link) {
		wl_list_remove(&mode_removing->link);
		free(mode_removing);
	}
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_add),
		cmocka_unit_test(test_optimal_mode_preferred),
	};
	// TODO common setup and teardown
	return cmocka_run_group_tests(tests, NULL, NULL);
}

