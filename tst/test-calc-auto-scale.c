#include "calc.h"

struct StateAutoScale {
	struct Head *head;
};

static int auto_scale_setup(void **state) {
	struct StateAutoScale *s = calloc(1, sizeof(struct StateAutoScale));

	s->head = calloc(1, sizeof(struct Head));
	s->head->desired.mode = calloc(1, sizeof(struct Mode));
	s->head->size_specified = true;

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
	assert_int_equal(scale, wl_fixed_from_int(1));

	// null desired.mode
	struct Mode *mode = s->head->desired.mode;
	s->head->desired.mode = NULL;
	scale = auto_scale(s->head);
	assert_int_equal(scale, wl_fixed_from_int(1));
	s->head->desired.mode = mode;

	// size not specified
	s->head->size_specified = false;
	scale = auto_scale(s->head);
	s->head->size_specified = true;
	assert_int_equal(scale, wl_fixed_from_int(1));

	// zero width_mm
	scale = auto_scale(s->head);
	assert_int_equal(scale, wl_fixed_from_int(1));

	// zero height_mm
	s->head->width_mm = 1;
	scale = auto_scale(s->head);
	assert_int_equal(scale, wl_fixed_from_int(1));

	// zero desired width
	s->head->height_mm = 1;
	scale = auto_scale(s->head);
	assert_int_equal(scale, wl_fixed_from_int(1));

	// zero desired height
	s->head->desired.mode->width = 1;
	scale = auto_scale(s->head);
	assert_int_equal(scale, wl_fixed_from_int(1));
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

#define calc_auto_scale_tests \
	cmocka_unit_test_setup_teardown(auto_scale_missing, auto_scale_setup, auto_scale_teardown), \
	cmocka_unit_test_setup_teardown(auto_scale_valid, auto_scale_setup, auto_scale_teardown)
