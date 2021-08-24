#include "util.h"

struct O {
	struct wl_list modes;
	struct Mode *mode12at3;
	struct Mode *mode34at1;
	struct Mode *mode34at2;
};

static int optimal_mode_setup(void **state) {
	struct O *o = calloc(1, sizeof(*o));

	struct Mode *mode;

	wl_list_init(&o->modes);

	mode = calloc(1, sizeof(*mode));
	o->mode12at3 = mode;
	wl_list_insert(&o->modes, &mode->link);

	mode = calloc(1, sizeof(*mode));
	o->mode34at1 = mode;
	wl_list_insert(&o->modes, &mode->link);

	mode = calloc(1, sizeof(*mode));
	o->mode34at2 = mode;
	wl_list_insert(&o->modes, &mode->link);

	*state = o;

	return 0;
}

static int optimal_mode_teardown(void **state) {
	struct O *o = *state;

	struct Mode *mode_removing, *mode_tmp;
	wl_list_for_each_safe(mode_removing, mode_tmp, &o->modes, link) {
		wl_list_remove(&mode_removing->link);
		free(mode_removing);
	}

	free(o);

	return 0;
}

static void optimal_mode_preferred(void **state) {
	struct O *o = *state;

	o->mode12at3->preferred = true;

	struct Mode *mode = optimal_mode(&o->modes);
	assert_ptr_equal(mode, o->mode12at3);
}

static void optimal_mode_highest(void **state) {
	struct O *o = *state;

	struct Mode *mode = optimal_mode(&o->modes);
	assert_ptr_equal(mode, o->mode34at2);
}

#define util_tests \
	cmocka_unit_test_setup_teardown(optimal_mode_highest, optimal_mode_setup, optimal_mode_teardown), \
	cmocka_unit_test_setup_teardown(optimal_mode_preferred, optimal_mode_setup, optimal_mode_teardown)

