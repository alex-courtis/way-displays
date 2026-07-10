#include "tst.h"

#include "assert-log.h"
#include "assert-mode.h"
#include "assert-pset.h"
#include "asserts.h"
#include "util-col.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdlib.h>

#include "fn.h"
#include "head.h"
#include "pset.h"
#include "smap.h"

#include "mode.h"

struct Mode *target = NULL;
const struct PSet *mset = NULL;
const struct SMap *mmap = NULL;

static int before_each(void **state) {
	// equal pointers, not vals, to match head->modes
	mmap = mode_smap_ptr_init();
	smap_put_many(mmap,
			"200x100x59999",  mode_whr(200, 100, 59999),
			"200x100x60498",  mode_whr(200, 100, 60498),
			"200x100x60499",  mode_whr(200, 100, 60499),
			"200x100x60500",  mode_whr(200, 100, 60500),
			"400x200x120000", mode_whr(400, 200, 120000),
			"600x600x164999", mode_whr(600, 300, 164999),
			"800x400x144000", mode_whr(800, 400, 144000),
			NULL);

	mset = smap_vals_pset(mmap);

	return 0;
}

static int after_each(void **state) {
	assert_logs_empty();

	mode_free(target);
	target = NULL;

	smap_free(mmap);
	pset_free_vals(mset);

	return 0;
}

// TODO test clone

// TODO put these in mset
static void mode_equal__all(void **state) {
	struct Mode *modea = mode_whr(1000, 1000, 1000);
	struct Mode *modeb0 = mode_whr(1000, 2000, 3000);
	struct Mode *modeb1 = mode_whr(1000, 2000, 4000);
	struct Mode *modeb2 = mode_whr(1000, 2000, 4100);

	assert_false(mode_equal_res(modea, modeb0));
	assert_true(mode_equal_res(modeb0, modeb1));

	assert_false(mode_equal_res_hz(modeb0, modeb1));
	assert_true(mode_equal_res_hz(modeb1, modeb2));

	assert_false(mode_equal_res_mhz(modeb0, modeb1));
	assert_false(mode_equal_res_mhz(modeb1, modeb2));
	assert_true(mode_equal_res_mhz(modeb2, modeb2));

	assert_false(mode_equal(modeb1, modeb2));
	assert_true(mode_equal(modeb2, modeb2));

	mode_free(modea);
	mode_free(modeb0);
	mode_free(modeb1);
	mode_free(modeb2);
}

static void mode_satisfies__null(void **state) {
	assert_false(mode_satisfies(NULL, NULL));
	assert_false(mode_satisfies(smap_get(mmap, "200x100x60499"), NULL));
	assert_false(mode_satisfies(NULL, smap_get(mmap, "200x100x60499")));
}

static void mode_greater_than_res_refresh__sort(void **state) {
	const struct Mode *mode00 = mode_whr(1000, 2000, 3000);
	const struct Mode *mode01 = mode_whr(1000, 9999, 3000);
	const struct Mode *mode02 = mode_whr(1000, 2000, 9999);
	const struct Mode *mode03 = mode_whr(9999, 2000, 3000);
	const struct Mode *mode04 = mode_whr(1000, 2000, 3000);

	const struct PSet *expected = mode_pset_ptr_init();
	pset_add_many(expected,
			mode03,
			mode01,
			mode02,
			mode00,
			mode04,
			NULL);

	const struct PSet *actual = mode_pset_ptr_init();
	pset_add_many(actual,
			mode00,
			mode01,
			mode02,
			mode03,
			mode04,
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

	const struct Mode *actual = mode_best_satisfying(target, mset);

	assert_mode_equal(actual, smap_get(mmap, "800x400x144000"));
}

static void mode_best_satisfying__no_hz_no_match(void **state) {
	target = mode_whr(999, 999, -1);

	assert_nul(mode_best_satisfying(target, mset));
}

static void mode_best_satisfying__no_hz_match(void **state) {
	target = mode_whr(400, 200, -1);

	assert_mode_equal(mode_best_satisfying(target, mset), smap_get(mmap, "400x200x120000"));
}

static void mode_best_satisfying__even_hz_no_match(void **state) {
	target = mode_whr(200, 100, 144000);

	assert_nul(mode_best_satisfying(target, mset));
}

static void mode_best_satisfying__even_hz_match(void **state) {
	target = mode_whr(200, 100, 60000);

	assert_mode_equal(mode_best_satisfying(target, mset), smap_get(mmap, "200x100x60499"));
}

static void mode_best_satisfying__even_hz_rounded_up(void **state) {
	target = mode_whr(600, 300, 165000);

	assert_mode_equal(mode_best_satisfying(target, mset), smap_get(mmap, "600x600x164999"));
}

static void mode_best_satisfying__exact_hz_match(void **state) {
	target = mode_whr(200, 100, 60498);

	assert_mode_equal(mode_best_satisfying(target, mset), smap_get(mmap, "200x100x60498"));
}

static void mode_best_satisfying__width_failed(void **state) {
	target = mode_whr(1000, 100, 60499);

	assert_nul(mode_best_satisfying(target, mset));
}

static void mode_best_satisfying__height_failed(void **state) {
	target = mode_whr(200, 9999999, 60499);

	assert_nul(mode_best_satisfying(target, mset));
}

static void mode_dpi__(void **state) {
	struct Head *head = head_init();
	head->width_mm = 1000;
	head->height_mm = 500;

	const struct Mode *mode = mode_h_whr(head, 2000, 1000, 0);
	pset_add(head->modes, mode);

	// nice roundish number to prevent odd test fails
	double expected = 50.8;

	double actual = mode_dpi(mode);

	assert_float_equal(actual, expected, 0);

	head_free(head);
}

static void mode_max_refresh__no_target(void **state) {
	const struct Mode *actual = mode_max_refresh(NULL, mset);

	assert_nul(actual);
}

static void mode_max_refresh__first_matches(void **state) {
	struct Mode *first = mode_whr(111, 222, 333);
	pset_add(mset, first);

	const struct Mode *second = mode_whr(111, 222, 333);
	pset_add(mset, second);

	target = mode_whr(111, 222, 333);

	// ensure we get the first pointer, not just an equal mode
	assert_ptr_equal(mode_max_refresh(target, mset), first);
}

static void mode_max_refresh__later_higher_refresh(void **state) {
	const struct Mode *lower = mode_whr(111, 222, 333);
	pset_add(mset, lower);

	const struct Mode *higher = mode_whr(111, 222, 999999);
	pset_add(mset, higher);

	target = mode_whr(111, 222, 333);

	assert_mode_equal(mode_max_refresh(target, mset), higher);
}

static void mode_max_refresh__earlier_higher_refresh(void **state) {
	const struct Mode *higher = mode_whr(111, 222, 999999);
	pset_add(mset, higher);

	const struct Mode *lower = mode_whr(111, 222, 333);
	pset_add(mset, lower);

	target = mode_whr(111, 222, 333);

	assert_mode_equal(mode_max_refresh(target, mset), higher);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(mode_equal__all),
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
		TEST_BA(mode_max_refresh__first_matches),
		TEST_BA(mode_max_refresh__later_higher_refresh),
		TEST_BA(mode_max_refresh__earlier_higher_refresh),
	};

	return RUN(tests);
}

