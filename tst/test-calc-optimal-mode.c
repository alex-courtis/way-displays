#include "calc.h"

struct StateOptimalMode {
	struct SList *modes;
	struct Mode *mode12at3;
	struct Mode *mode34at1;
	struct Mode *mode34at2;
};

static int optimal_mode_setup(void **state) {
	struct StateOptimalMode *s = calloc(1, sizeof(struct StateOptimalMode));

	struct Mode *mode;

	mode = calloc(1, sizeof(struct Mode));
	mode->width = 1;
	mode->height = 2;
	mode->refresh_mHz = 3;
	s->mode12at3 = mode;
	slist_append(&s->modes, mode);

	slist_append(&s->modes, NULL);

	mode = calloc(1, sizeof(struct Mode));
	s->mode34at1 = mode;
	mode->width = 3;
	mode->height = 4;
	mode->refresh_mHz = 1;
	slist_append(&s->modes, mode);

	mode = calloc(1, sizeof(struct Mode));
	mode->width = 3;
	mode->height = 4;
	mode->refresh_mHz = 2;
	s->mode34at2 = mode;
	slist_append(&s->modes, mode);

	*state = s;

	return 0;
}

static int optimal_mode_teardown(void **state) {
	struct StateOptimalMode *s = *state;

	for (struct SList *i = s->modes; i; i = i->nex) {
		free(i->val);
	}
	slist_free(&s->modes);
	free(s);

	return 0;
}

static void optimal_mode_preferred(void **state) {
	struct StateOptimalMode *s = *state;

	s->mode12at3->preferred = true;

	struct Mode *mode = optimal_mode(s->modes);
	assert_ptr_equal(mode, s->mode12at3);
}

static void optimal_mode_highest(void **state) {
	struct StateOptimalMode *s = *state;

	struct Mode *mode = optimal_mode(s->modes);
	assert_ptr_equal(mode, s->mode34at2);
}

#define calc_optimal_mode_tests \
	cmocka_unit_test_setup_teardown(optimal_mode_highest, optimal_mode_setup, optimal_mode_teardown), \
	cmocka_unit_test_setup_teardown(optimal_mode_preferred, optimal_mode_setup, optimal_mode_teardown)
