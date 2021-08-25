#include "util.h"

struct StateOptimalMode {
	struct wl_list modes;
	struct Mode *mode12at3;
	struct Mode *mode34at1;
	struct Mode *mode34at2;
};

static int optimal_mode_setup(void **state) {
	struct StateOptimalMode *s = calloc(1, sizeof(struct StateOptimalMode));

	struct Mode *mode;

	wl_list_init(&s->modes);

	mode = calloc(1, sizeof(struct Mode));
	s->mode12at3 = mode;
	wl_list_insert(&s->modes, &mode->link);

	mode = calloc(1, sizeof(struct Mode));
	s->mode34at1 = mode;
	wl_list_insert(&s->modes, &mode->link);

	mode = calloc(1, sizeof(struct Mode));
	s->mode34at2 = mode;
	wl_list_insert(&s->modes, &mode->link);

	*state = s;

	return 0;
}

static int optimal_mode_teardown(void **state) {
	struct StateOptimalMode *s = *state;

	struct Mode *mode_removing, *mode_tmp;
	wl_list_for_each_safe(mode_removing, mode_tmp, &s->modes, link) {
		wl_list_remove(&mode_removing->link);
		free(mode_removing);
	}

	free(s);

	return 0;
}

static void optimal_mode_preferred(void **state) {
	struct StateOptimalMode *s = *state;

	s->mode12at3->preferred = true;

	struct Mode *mode = optimal_mode(&s->modes);
	assert_ptr_equal(mode, s->mode12at3);
}

static void optimal_mode_highest(void **state) {
	struct StateOptimalMode *s = *state;

	struct Mode *mode = optimal_mode(&s->modes);
	assert_ptr_equal(mode, s->mode34at2);
}

struct StateAutoScale {
	struct Head *head;
};

static int auto_scale_setup(void **state) {
	struct StateAutoScale *s = calloc(1, sizeof(struct StateAutoScale));

	s->head = calloc(1, sizeof(struct Head));
	s->head->desired.mode = calloc(1, sizeof(struct Mode));

	*state = s;

	return 0;
}

static int auto_scale_teardown(void **state) {
	struct StateAutoScale *s = *state;

	free(s->head->desired.mode);
	free(s->head);

	free(s);

	return 0;
}
static void auto_scale_missing(void **state) {
	struct StateAutoScale *s = *state;

	// null head
	wl_fixed_t scale = auto_scale(NULL);
	assert_int_equal(scale, wl_fixed_from_int(0));

	// null desired.mode
	struct Mode *mode = s->head->desired.mode;
	s->head->desired.mode = NULL;
	scale = auto_scale(s->head);
	assert_int_equal(scale, wl_fixed_from_int(0));
	s->head->desired.mode = mode;

	// zero width_mm
	scale = auto_scale(s->head);
	assert_int_equal(scale, wl_fixed_from_int(0));

	// zero height_mm
	s->head->width_mm = 1;
	scale = auto_scale(s->head);
	assert_int_equal(scale, wl_fixed_from_int(0));

	// zero desired width
	s->head->height_mm = 1;
	scale = auto_scale(s->head);
	assert_int_equal(scale, wl_fixed_from_int(0));

	// zero desired height
	s->head->desired.mode->width = 1;
	scale = auto_scale(s->head);
	assert_int_equal(scale, wl_fixed_from_int(0));
}

static void auto_scale_valid(void **state) {
	struct StateAutoScale *s = *state;

	// dpi 93.338022 which rounds to 96, a scale of 1
	s->head->width_mm = 700;
	s->head->height_mm = 390;
	s->head->desired.mode->width = 2560;
	s->head->desired.mode->height = 1440;
	wl_fixed_t scale = auto_scale(s->head);
	assert_int_equal(scale, wl_fixed_from_int(1));

	// dpi 287.814241 which rounds to 288, a scale of 3
	s->head->width_mm = 340;
	s->head->height_mm = 190;
	s->head->desired.mode->width = 3840;
	s->head->desired.mode->height = 2160;
	scale = auto_scale(s->head);
	assert_int_equal(scale, wl_fixed_from_int(3));

	// dpi 212.453890 which rounds to 216, a scale of 2.25
	s->head->width_mm = 310;
	s->head->height_mm = 170;
	s->head->desired.mode->width = 2560;
	s->head->desired.mode->height = 1440;
	scale = auto_scale(s->head);
	assert_int_equal(scale, wl_fixed_from_double(2.25));
}

#define util_tests \
	cmocka_unit_test_setup_teardown(optimal_mode_highest, optimal_mode_setup, optimal_mode_teardown), \
	cmocka_unit_test_setup_teardown(optimal_mode_preferred, optimal_mode_setup, optimal_mode_teardown), \
	cmocka_unit_test_setup_teardown(auto_scale_missing, auto_scale_setup, auto_scale_teardown), \
	cmocka_unit_test_setup_teardown(auto_scale_valid, auto_scale_setup, auto_scale_teardown)

