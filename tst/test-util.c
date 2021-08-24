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

static void auto_scale_missing(void **state) {
	struct Head head;
	struct Mode mode;

	wl_fixed_t scale = auto_scale(NULL);
	assert_int_equal(scale, wl_fixed_from_int(0));

	scale = auto_scale(&head);
	assert_int_equal(scale, wl_fixed_from_int(0));

	head.desired_mode = &mode;
	scale = auto_scale(&head);
	assert_int_equal(scale, wl_fixed_from_int(0));

	head.width_mm = 1;
	scale = auto_scale(&head);
	assert_int_equal(scale, wl_fixed_from_int(0));

	head.width_mm = 0;
	head.height_mm = 1;
	scale = auto_scale(&head);
	assert_int_equal(scale, wl_fixed_from_int(0));
}

static void auto_scale_valid(void **state) {
	struct Head head;
	struct Mode mode;

	head.desired_mode = &mode;

	// dpi 93.338022 which rounds to 96, a scale of 1
	head.width_mm = 700;
	head.height_mm = 390;
	mode.width = 2560;
	mode.height = 1440;
	wl_fixed_t scale = auto_scale(&head);
	assert_int_equal(scale, wl_fixed_from_int(1));

	// dpi 287.814241 which rounds to 288, a scale of 3
	head.width_mm = 340;
	head.height_mm = 190;
	mode.width = 3840;
	mode.height = 2160;
	scale = auto_scale(&head);
	assert_int_equal(scale, wl_fixed_from_int(3));

	// dpi 212.453890 which rounds to 216, a scale of 2.25
	head.width_mm = 310;
	head.height_mm = 170;
	mode.width = 2560;
	mode.height = 1440;
	scale = auto_scale(&head);
	assert_int_equal(scale, wl_fixed_from_double(2.25));
}

#define util_tests \
	cmocka_unit_test_setup_teardown(optimal_mode_highest, optimal_mode_setup, optimal_mode_teardown), \
	cmocka_unit_test_setup_teardown(optimal_mode_preferred, optimal_mode_setup, optimal_mode_teardown), \
	cmocka_unit_test(auto_scale_missing), \
	cmocka_unit_test(auto_scale_valid)

