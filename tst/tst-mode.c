#include "tst.h"

#include "assert-log.h"
#include "assert-mode.h"
#include "assert-pset.h"
#include "asserts.h"
#include "data.h"
#include "util-col.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "fn.h"
#include "head.h"
#include "ppmap.h"
#include "pset.h"
#include "spmap.h"

#include "mode.h"

struct Mode *target = NULL;
const struct PPmap *head_modes = NULL;
const struct SPmap *cfg_modes = NULL;

static int before_each(void **state) {

	cfg_modes = mode_spmap_init();
	spmap_put_many(cfg_modes,
			"200x100@59999",  mode_whr(200, 100, 59999),
			"200x100@60498",  mode_whr(200, 100, 60498),
			"200x100@60499",  mode_whr(200, 100, 60499),
			"200x100@60500",  mode_whr(200, 100, 60500),
			"400x200@120000", mode_whr(400, 200, 120000),
			"600x600@164999", mode_whr(600, 300, 164999),
			"800x400@144000", mode_whr(800, 400, 144000),
			"1x1@1000",       mode_whr(1, 1, 1000),
			"2x1@1000",       mode_whr(2, 1, 1000),
			"1x2@3000",       mode_whr(1, 2, 3000),
			"1x2@4000",       mode_whr(1, 2, 4000),
			"1x2@4100",       mode_whr(1, 2, 4100),
			NULL);

	head_modes = mode_ppmap_init();
	for (const struct SPmapIt *it = spmap_it(cfg_modes); it; it = spmap_it_next(it)) {
		ppmap_put(head_modes, it->key, it->val);
	}

	return 0;
}

static int after_each(void **state) {
	assert_logs_empty();

	mode_free(target);
	target = NULL;

	spmap_free(cfg_modes);
	ppmap_free_vals(head_modes);

	return 0;
}

static void mode_clone__(void **state) {
	assert_nul(mode_clone(NULL));

	struct Head head = { 0 };
	struct zwlr_output_mode_v1 *zwlr_mode = NULL;

	struct Mode from = {
		.head = &head,
		.zwlr_mode = zwlr_mode,
		.width = 2,
		.height = 1,
		.refresh_mhz = 3,
		.max = true,
		.warned_no_mode = true,
	};

	struct Mode *to = mode_clone(&from);

	assert_ptr_equal(to->head, &head);
	assert_ptr_equal(to->zwlr_mode, zwlr_mode);
	assert_int_equal(to->width, 2);
	assert_int_equal(to->height, 1);
	assert_int_equal(to->refresh_mhz, 3);
	assert_true(to->max);
	assert_true(to->warned_no_mode);

	mode_free(to);
}

static void mode_equal__variants(void **state) {
	assert_false(mode_equal_res(spmap_get(cfg_modes, "1x1@1000"), spmap_get(cfg_modes, "1x2@3000")));
	assert_false(mode_equal_res(spmap_get(cfg_modes, "1x1@1000"), spmap_get(cfg_modes, "2x1@3000")));

	assert_true (mode_equal_res(spmap_get(cfg_modes, "1x2@3000"), spmap_get(cfg_modes, "1x2@4000")));


	assert_false(mode_equal_res_hz(spmap_get(cfg_modes, "1x2@3000"), spmap_get(cfg_modes, "1x2@4000")));

	assert_true (mode_equal_res_hz(spmap_get(cfg_modes, "1x2@4000"), spmap_get(cfg_modes, "1x2@4100")));


	assert_false(mode_equal_res_mhz(spmap_get(cfg_modes, "1x2@3000"), spmap_get(cfg_modes, "1x2@4000")));
	assert_false(mode_equal_res_mhz(spmap_get(cfg_modes, "1x2@4000"), spmap_get(cfg_modes, "1x2@4100")));

	assert_true (mode_equal_res_mhz(spmap_get(cfg_modes, "1x2@4100"), spmap_get(cfg_modes, "1x2@4100")));


	assert_false(mode_equal(spmap_get(cfg_modes, "1x2@4000"), spmap_get(cfg_modes, "1x2@4100")));

	assert_true (mode_equal(spmap_get(cfg_modes, "1x2@4100"), spmap_get(cfg_modes, "1x2@4100")));
}

static void mode_satisfies__null(void **state) {
	assert_false(mode_satisfies(NULL, NULL));
	assert_false(mode_satisfies(spmap_get(cfg_modes, "200x100@60499"), NULL));
	assert_false(mode_satisfies(NULL, spmap_get(cfg_modes, "200x100@60499")));
}

static void mode_greater_than_res_refresh__sort(void **state) {
	const struct Mode *mode06 = mode_whr(9, 2, 3);
	const struct Mode *mode05 = mode_whr(2, 2, 9);
	const struct Mode *mode04 = mode_whr(2, 2, 8);
	const struct Mode *mode03 = mode_whr(2, 1, 9);
	const struct Mode *mode02 = mode_whr(1, 9, 3);
	const struct Mode *mode01 = mode_whr(1, 2, 9);
	const struct Mode *mode00 = mode_whr(1, 2, 3);

	const struct PsetParams params = {
		.free_val = (fn_free)mode_free,
		.str_val = (fn_str)mode_str,
	};
	const struct Pset *expected = pset_init_with(params);
	pset_add_many(expected,
			mode06,
			mode05,
			mode04,
			mode03,
			mode02,
			mode01,
			mode00,
			NULL);

	const struct Pset *actual = pset_init();
	pset_add_many(actual,
			mode00,
			mode01,
			mode02,
			mode03,
			mode05,
			mode04,
			mode06,
			NULL);

	pset_sort(actual, (fn_less_than)mode_greater_than_res_refresh);

	assert_pset_equal(actual, expected);

	pset_free_vals(expected);
	pset_free(actual);
}

static void mode_hz_rounded__(void **state) {
	target = mode_init();

	target->refresh_mhz = 0;
	assert_int_equal(mode_hz_rounded(target), 0);

	target->refresh_mhz = 123567;
	assert_int_equal(mode_hz_rounded(target), 124);

	target->refresh_mhz = 123500;
	assert_int_equal(mode_hz_rounded(target), 124);
}

static void mode_best_satisfying__max(void **state) {
	target = mode_whr_max(-1, -1, -1);

	const struct Mode *actual = mode_best_satisfying(target, head_modes);

	assert_mode_equal(actual, spmap_get(cfg_modes, "800x400@144000"));
}

static void mode_best_satisfying__no_hz_no_match(void **state) {
	target = mode_whr(999, 999, -1);

	assert_nul(mode_best_satisfying(target, head_modes));
}

static void mode_best_satisfying__no_hz_match(void **state) {
	target = mode_whr(400, 200, -1);

	assert_mode_equal(mode_best_satisfying(target, head_modes), spmap_get(cfg_modes, "400x200@120000"));
}

static void mode_best_satisfying__even_hz_no_match(void **state) {
	target = mode_whr(200, 100, 144000);

	assert_nul(mode_best_satisfying(target, head_modes));
}

static void mode_best_satisfying__even_hz_match(void **state) {
	target = mode_whr(200, 100, 60000);

	assert_mode_equal(mode_best_satisfying(target, head_modes), spmap_get(cfg_modes, "200x100@60499"));
}

static void mode_best_satisfying__even_hz_rounded_up(void **state) {
	target = mode_whr(600, 300, 165000);

	assert_mode_equal(mode_best_satisfying(target, head_modes), spmap_get(cfg_modes, "600x600@164999"));
}

static void mode_best_satisfying__exact_hz_match(void **state) {
	target = mode_whr(200, 100, 60498);

	assert_mode_equal(mode_best_satisfying(target, head_modes), spmap_get(cfg_modes, "200x100@60498"));
}

static void mode_best_satisfying__width_failed(void **state) {
	target = mode_whr(1000, 100, 60499);

	assert_nul(mode_best_satisfying(target, head_modes));
}

static void mode_best_satisfying__height_failed(void **state) {
	target = mode_whr(200, 9999999, 60499);

	assert_nul(mode_best_satisfying(target, head_modes));
}

static void mode_dpi__(void **state) {
	struct Head *head = head_init();
	head->width_mm = 1000;
	head->height_mm = 500;

	struct Mode *mode = mode_h_whr(head, 2000, 1000, 0);

	// nice roundish number to prevent odd test fails
	double expected = 50.8;

	double actual = mode_dpi(mode);

	assert_float_equal(actual, expected, 0);

	mode_free(mode);
	head_free(head);
}

static void mode_max_refresh__no_target(void **state) {
	const struct Mode *actual = mode_max_refresh(NULL, head_modes);

	assert_nul(actual);
}

static void mode_max_refresh__exact_matches(void **state) {
	const struct Mode *exact = mode_whr(111, 222, 333);
	ppmap_put(head_modes, M0, exact);

	target = mode_whr(111, 222, 333);

	assert_mode_equal(mode_max_refresh(target, head_modes), exact);
}

static void mode_max_refresh__later_higher_refresh(void **state) {
	const struct Mode *lower = mode_whr(111, 222, 333);
	ppmap_put(head_modes, M0, lower);

	const struct Mode *higher = mode_whr(111, 222, 999999);
	ppmap_put(head_modes, M1, higher);

	target = mode_whr(111, 222, 333);

	assert_mode_equal(mode_max_refresh(target, head_modes), higher);
}

static void mode_max_refresh__earlier_higher_refresh(void **state) {
	const struct Mode *higher = mode_whr(111, 222, 999999);
	ppmap_put(head_modes, M0, higher);

	const struct Mode *lower = mode_whr(111, 222, 333);
	ppmap_put(head_modes, M1, lower);

	target = mode_whr(111, 222, 333);

	assert_mode_equal(mode_max_refresh(target, head_modes), higher);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(mode_clone__),
		TEST_BA(mode_equal__variants),
		TEST_BA(mode_satisfies__null),

		TEST_BA(mode_greater_than_res_refresh__sort),

		TEST_BA(mode_hz_rounded__),

		TEST_BA(mode_best_satisfying__max),
		TEST_BA(mode_best_satisfying__no_hz_no_match),
		TEST_BA(mode_best_satisfying__no_hz_match),
		TEST_BA(mode_best_satisfying__even_hz_no_match),
		TEST_BA(mode_best_satisfying__even_hz_match),
		TEST_BA(mode_best_satisfying__even_hz_rounded_up),
		TEST_BA(mode_best_satisfying__exact_hz_match),
		TEST_BA(mode_best_satisfying__width_failed),
		TEST_BA(mode_best_satisfying__height_failed),

		TEST_BA(mode_dpi__),

		TEST_BA(mode_max_refresh__no_target),
		TEST_BA(mode_max_refresh__exact_matches),
		TEST_BA(mode_max_refresh__later_higher_refresh),
		TEST_BA(mode_max_refresh__earlier_higher_refresh),
	};

	return RUN(tests);
}

