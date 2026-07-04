#include "tst.h"

#include "assert-log.h"
#include "assert-mode.h"
#include "assert-pset.h"
#include "asserts.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdlib.h>

#include "fn.h"
#include "head.h"
#include "pset.h"

#include "mode.h"

struct Mode *user_mode = NULL;
const struct PSet *modes = NULL;
const struct PSet *modes_failed = NULL;

struct Mode *mode0, *mode1, *mode2, *mode3, *mode4, *mode5;

static int before_each(void **state) {
	assert_logs_empty_before();

	mode0 = mode_init_whr(200, 100, 59999);
	mode1 = mode_init_whr(200, 100, 60499);
	mode2 = mode_init_whr(200, 100, 60500);
	mode3 = mode_init_whr(400, 200, 120000);
	mode4 = mode_init_whr(600, 300, 164999);
	mode5 = mode_init_whr(800, 400, 144000);

	modes = mode_pset_init();
	modes_failed = mode_pset_init();

	pset_add(modes, mode0);
	pset_add(modes, mode1);
	pset_add(modes, mode2);
	pset_add(modes, mode3);
	pset_add(modes, mode4);
	pset_add(modes, mode5);

	return 0;
}

static int after_each(void **state) {
	free(user_mode);
	user_mode = NULL;

	pset_free_vals(modes);
	pset_free(modes_failed);

	return 0;
}

static void mode__sort(void **state) {
	const struct Mode *mode00 = mode_init_whr(1000, 2000, 3000);
	const struct Mode *mode01 = mode_init_whr(1000, 9999, 3000);
	const struct Mode *mode02 = mode_init_whr(1000, 2000, 9999);
	const struct Mode *mode03 = mode_init_whr(9999, 2000, 3000);
	const struct Mode *mode04 = mode_init_whr(1000, 2000, 3000);

	const struct PSet *expected = mode_pset_init();
	pset_add(expected, mode03);
	pset_add(expected, mode01);
	pset_add(expected, mode02);
	pset_add(expected, mode00);
	pset_add(expected, mode04);

	const struct PSet *actual = mode_pset_init();
	pset_add(actual, mode00);
	pset_add(actual, mode01);
	pset_add(actual, mode02);
	pset_add(actual, mode03);
	pset_add(actual, mode04);

	pset_sort(actual, (fn_less_than)mode_greater_than_res_refresh);

	assert_pset_equal(actual, expected);

	pset_free_vals(expected);
	pset_free(actual);
}

static void mode_mhz_to_hz_rounded__(void **state) {
	assert_int_equal(mhz_to_hz_rounded(0), 0);
	assert_int_equal(mhz_to_hz_rounded(123567), 124);
}

static void mode_user_mode__max(void **state) {
	user_mode = mode_init_whr_max(-1, -1, -1);

	const struct Mode *actual = mode_for_user_mode(modes, modes_failed, user_mode);

	assert_mode_equal(actual, mode5);
	assert_ptr_equal(actual, mode5);
}

static void mode_user_mode__no_hz_no_match(void **state) {
	user_mode = mode_init_whr(999, 999, -1);

	const struct Mode *actual = mode_for_user_mode(modes, modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_user_mode__no_hz_match(void **state) {
	user_mode = mode_init_whr(400, 200, -1);

	const struct Mode *actual = mode_for_user_mode(modes, modes_failed, user_mode);

	assert_mode_equal(actual, mode3);
	assert_ptr_equal(actual, mode3);
}

static void mode_user_mode__even_hz_no_match(void **state) {
	user_mode = mode_init_whr(200, 100, 144000);

	const struct Mode *actual = mode_for_user_mode(modes, modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_user_mode__even_hz_match(void **state) {
	user_mode = mode_init_whr(200, 100, 60000);

	const struct Mode *actual = mode_for_user_mode(modes, modes_failed, user_mode);

	assert_mode_equal(actual, mode1);
	assert_ptr_equal(actual, mode1);
}

static void mode_user_mode__even_hz_rounded_up(void **state) {
	user_mode = mode_init_whr(600, 300, 165000);

	const struct Mode *actual = mode_for_user_mode(modes, modes_failed, user_mode);

	assert_mode_equal(actual, mode4);
	assert_ptr_equal(actual, mode4);
}

static void mode_user_mode__failed(void **state) {
	user_mode = mode_init_whr(200, 100, 60000);

	pset_add(modes_failed, mode1);

	const struct Mode *actual = mode_for_user_mode(modes, modes_failed, user_mode);

	assert_mode_equal(actual, mode0);
	assert_ptr_equal(actual, mode0);
}

static void mode_user_mode__exact_hz_match(void **state) {
	user_mode = mode_init_whr(200, 100, 60499);

	const struct Mode *actual = mode_for_user_mode(modes, modes_failed, user_mode);

	assert_mode_equal(actual, mode1);
	assert_ptr_equal(actual, mode1);
}

static void mode_user_mode__exact_hz_failed(void **state) {
	user_mode = mode_init_whr(200, 100, 60499);

	pset_add(modes_failed, mode1);

	const struct Mode *actual = mode_for_user_mode(modes, modes_failed, user_mode);

	assert_mode_equal(actual, mode0);
	assert_ptr_equal(actual, mode0);
}

static void mode_user_mode__width_failed(void **state) {
	user_mode = mode_init_whr(1000, 100, 60499);

	const struct Mode *actual = mode_for_user_mode(modes, modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_user_mode__height_failed(void **state) {
	user_mode = mode_init_whr(200, 9999999, 60499);

	const struct Mode *actual = mode_for_user_mode(modes, modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_user_mode__all_matches_failed(void **state) {
	user_mode = mode_init_whr(200, 100, 60499);

	pset_add(modes_failed, mode0);
	pset_add(modes_failed, mode1);

	const struct Mode *actual = mode_for_user_mode(modes, modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_dpi__(void **state) {
	struct Head *head = head_init();
	head->width_mm = 1000;
	head->height_mm = 500;

	const struct Mode *mode = mode_init_h_whr(head, 2000, 1000, 0);
	pset_add(head->modes, mode);

	// nice roundish number to prevent odd test fails
	double expected = 50.8;

	double actual = mode_dpi(mode);

	assert_float_equal(actual, expected, 0);

	head_free(head);
}

static void mode_preferred__no_preferred(void **state) {
	const struct Mode *actual = mode_preferred(modes, NULL);

	assert_nul(actual);
}

static void mode_preferred__preferred(void **state) {
	struct Mode *expected = mode_init_whr_pref(111, 222, 333);
	pset_add(modes, expected);

	const struct Mode *actual = mode_preferred(modes, NULL);

	assert_mode_equal(actual, expected);
	assert_ptr_equal(actual, expected);
}

static void mode_preferred__preferred_failed(void **state) {
	const struct Mode *expected = mode_init_whr_pref(111, 222, 333);
	pset_add(modes, expected);

	pset_add(modes_failed, expected);

	const struct Mode *actual = mode_preferred(modes, modes_failed);

	assert_nul(actual);
}

static void mode_max_preferred__no_preferred(void **state) {
	const struct Mode *actual = mode_max_preferred(modes, NULL);

	assert_nul(actual);
}

static void mode_max_preferred__preferred_matches(void **state) {
	struct Mode *expected = mode_init_whr_pref(111, 222, 333);
	pset_add(modes, expected);

	const struct Mode *actual = mode_max_preferred(modes, NULL);

	assert_mode_equal(actual, expected);
	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__prior_matches(void **state) {
	struct Mode *expected = mode_init_whr(111, 222, 333);
	pset_add(modes, expected);

	const struct Mode *preferred = mode_init_whr_pref(111, 222, 333);
	pset_add(modes, preferred);

	const struct Mode *actual = mode_max_preferred(modes, NULL);

	assert_mode_equal(actual, expected);
	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__later_higher_refresh(void **state) {
	const struct Mode *preferred = mode_init_whr_pref(111, 222, 333);
	pset_add(modes, preferred);

	struct Mode *expected = mode_init_whr(111, 222, 999999);
	pset_add(modes, expected);

	const struct Mode *actual = mode_max_preferred(modes, NULL);

	assert_mode_equal(actual, expected);
	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__earlier_higher_refresh(void **state) {
	struct Mode *expected = mode_init_whr(111, 222, 999999);
	pset_add(modes, expected);

	const struct Mode *preferred = mode_init_whr_pref(111, 222, 333);
	pset_add(modes, preferred);

	const struct Mode *actual = mode_max_preferred(modes, NULL);

	assert_mode_equal(actual, expected);
	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__failed(void **state) {
	const struct Mode *failed = mode_init_whr(111, 222, 2000);
	pset_add(modes, failed);
	pset_add(modes_failed, failed);

	const struct Mode *preferred = mode_init_whr_pref(111, 222, 333);
	pset_add(modes, preferred);

	struct Mode *expected = mode_init_whr(111, 222, 1000);
	pset_add(modes, expected);

	const struct Mode *actual = mode_max_preferred(modes, modes_failed);

	assert_mode_equal(actual, expected);
	assert_ptr_equal(actual, expected);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(mode__sort),

		TEST_BA(mode_mhz_to_hz_rounded__),

		TEST_BA(mode_user_mode__max),
		TEST_BA(mode_user_mode__no_hz_no_match),
		TEST_BA(mode_user_mode__no_hz_match),
		TEST_BA(mode_user_mode__even_hz_no_match),
		TEST_BA(mode_user_mode__even_hz_match),
		TEST_BA(mode_user_mode__even_hz_rounded_up),
		TEST_BA(mode_user_mode__failed),
		TEST_BA(mode_user_mode__exact_hz_match),
		TEST_BA(mode_user_mode__exact_hz_failed),
		TEST_BA(mode_user_mode__width_failed),
		TEST_BA(mode_user_mode__height_failed),
		TEST_BA(mode_user_mode__all_matches_failed),

		TEST_BA(mode_dpi__),

		TEST_BA(mode_preferred__no_preferred),
		TEST_BA(mode_preferred__preferred),
		TEST_BA(mode_preferred__preferred_failed),

		TEST_BA(mode_max_preferred__no_preferred),
		TEST_BA(mode_max_preferred__preferred_matches),
		TEST_BA(mode_max_preferred__prior_matches),
		TEST_BA(mode_max_preferred__later_higher_refresh),
		TEST_BA(mode_max_preferred__earlier_higher_refresh),
		TEST_BA(mode_max_preferred__failed),
	};

	return RUN(tests);
}

