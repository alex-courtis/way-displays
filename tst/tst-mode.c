#include "tst.h"

#include "assert-log.h"
#include "assert-mode.h"
#include "assert-pset.h"
#include "asserts.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "fn.h"
#include "head.h"
#include "pset.h"

#include "mode.h"

struct WlrMode *user_mode = NULL;
const struct PSet *wlr_modes = NULL;
const struct PSet *wlr_modes_failed = NULL;

struct WlrMode *mode0, *mode1, *mode2, *mode3, *mode4, *mode5;

static int before_each(void **state) {
	assert_logs_empty_before();

	mode0 = wlr_mode_init_whr(200, 100, 59999);
	mode1 = wlr_mode_init_whr(200, 100, 60499);
	mode2 = wlr_mode_init_whr(200, 100, 60500);
	mode3 = wlr_mode_init_whr(400, 200, 120000);
	mode4 = wlr_mode_init_whr(600, 300, 164999);
	mode5 = wlr_mode_init_whr(800, 400, 144000);

	wlr_modes = wlr_mode_pset_init();
	wlr_modes_failed = wlr_mode_pset_init();

	pset_add(wlr_modes, mode0);
	pset_add(wlr_modes, mode1);
	pset_add(wlr_modes, mode2);
	pset_add(wlr_modes, mode3);
	pset_add(wlr_modes, mode4);
	pset_add(wlr_modes, mode5);

	return 0;
}

static int after_each(void **state) {
	free(user_mode);
	user_mode = NULL;

	pset_free_vals(wlr_modes);
	pset_free(wlr_modes_failed);

	return 0;
}

static void mode__sort(void **state) {
	const struct WlrMode *mode00 = wlr_mode_init_whr(1000, 2000, 3000);
	const struct WlrMode *mode01 = wlr_mode_init_whr(1000, 9999, 3000);
	const struct WlrMode *mode02 = wlr_mode_init_whr(1000, 2000, 9999);
	const struct WlrMode *mode03 = wlr_mode_init_whr(9999, 2000, 3000);
	const struct WlrMode *mode04 = wlr_mode_init_whr(1000, 2000, 3000);

	const struct PSet *expected = wlr_mode_pset_init();
	pset_add(expected, mode03);
	pset_add(expected, mode01);
	pset_add(expected, mode02);
	pset_add(expected, mode00);
	pset_add(expected, mode04);

	const struct PSet *actual = wlr_mode_pset_init();
	pset_add(actual, mode00);
	pset_add(actual, mode01);
	pset_add(actual, mode02);
	pset_add(actual, mode03);
	pset_add(actual, mode04);

	pset_sort(actual, (fn_less_than)wlr_mode_greater_than_res_refresh);

	assert_pset_equal(actual, expected);

	pset_free_vals(expected);
	pset_free(actual);
}

static void mode_mhz_to_hz_rounded__(void **state) {
	assert_int_equal(mhz_to_hz_rounded(0), 0);
	assert_int_equal(mhz_to_hz_rounded(123567), 124);
}

static void mode_user_mode__max(void **state) {
	user_mode = wlr_mode_init_whr_max(-1, -1, -1);

	const struct WlrMode *actual = wlr_mode_for_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_mode_equal(actual, mode5);
	assert_ptr_equal(actual, mode5);
}

static void mode_user_mode__no_hz_no_match(void **state) {
	user_mode = wlr_mode_init_whr(999, 999, -1);

	const struct WlrMode *actual = wlr_mode_for_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_user_mode__no_hz_match(void **state) {
	user_mode = wlr_mode_init_whr(400, 200, -1);

	const struct WlrMode *actual = wlr_mode_for_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_mode_equal(actual, mode3);
	assert_ptr_equal(actual, mode3);
}

static void mode_user_mode__even_hz_no_match(void **state) {
	user_mode = wlr_mode_init_whr(200, 100, 144000);

	const struct WlrMode *actual = wlr_mode_for_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_user_mode__even_hz_match(void **state) {
	user_mode = wlr_mode_init_whr(200, 100, 60000);

	const struct WlrMode *actual = wlr_mode_for_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_mode_equal(actual, mode1);
	assert_ptr_equal(actual, mode1);
}

static void mode_user_mode__even_hz_rounded_up(void **state) {
	user_mode = wlr_mode_init_whr(600, 300, 165000);

	const struct WlrMode *actual = wlr_mode_for_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_mode_equal(actual, mode4);
	assert_ptr_equal(actual, mode4);
}

static void mode_user_mode__failed(void **state) {
	user_mode = wlr_mode_init_whr(200, 100, 60000);

	pset_add(wlr_modes_failed, mode1);

	const struct WlrMode *actual = wlr_mode_for_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_mode_equal(actual, mode0);
	assert_ptr_equal(actual, mode0);
}

static void mode_user_mode__exact_hz_match(void **state) {
	user_mode = wlr_mode_init_whr(200, 100, 60499);

	const struct WlrMode *actual = wlr_mode_for_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_mode_equal(actual, mode1);
	assert_ptr_equal(actual, mode1);
}

static void mode_user_mode__exact_hz_failed(void **state) {
	user_mode = wlr_mode_init_whr(200, 100, 60499);

	pset_add(wlr_modes_failed, mode1);

	const struct WlrMode *actual = wlr_mode_for_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_mode_equal(actual, mode0);
	assert_ptr_equal(actual, mode0);
}

static void mode_user_mode__width_failed(void **state) {
	user_mode = wlr_mode_init_whr(1000, 100, 60499);

	const struct WlrMode *actual = wlr_mode_for_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_user_mode__height_failed(void **state) {
	user_mode = wlr_mode_init_whr(200, 9999999, 60499);

	const struct WlrMode *actual = wlr_mode_for_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_user_mode__all_matches_failed(void **state) {
	user_mode = wlr_mode_init_whr(200, 100, 60499);

	pset_add(wlr_modes_failed, mode0);
	pset_add(wlr_modes_failed, mode1);

	const struct WlrMode *actual = wlr_mode_for_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_dpi__(void **state) {
	struct Head *head = head_init();
	head->width_mm = 1000;
	head->height_mm = 500;

	const struct WlrMode *wlr_mode = wlr_mode_init_h_whr(head, 2000, 1000, 0);
	pset_add(head->wlr_modes, wlr_mode);

	// nice roundish number to prevent odd test fails
	double expected = 50.8;

	double actual = wlr_mode_dpi(wlr_mode);

	assert_float_equal(actual, expected, 0);

	head_free(head);
}

static void mode_preferred__no_preferred(void **state) {
	const struct WlrMode *actual = wlr_mode_preferred(wlr_modes, NULL);

	assert_nul(actual);
}

static void mode_preferred__preferred(void **state) {
	struct WlrMode *expected = wlr_mode_init_whr_pref(111, 222, 333);
	pset_add(wlr_modes, expected);

	const struct WlrMode *actual = wlr_mode_preferred(wlr_modes, NULL);

	assert_mode_equal(actual, expected);
	assert_ptr_equal(actual, expected);
}

static void mode_preferred__preferred_failed(void **state) {
	const struct WlrMode *expected = wlr_mode_init_whr_pref(111, 222, 333);
	pset_add(wlr_modes, expected);

	pset_add(wlr_modes_failed, expected);

	const struct WlrMode *actual = wlr_mode_preferred(wlr_modes, wlr_modes_failed);

	assert_nul(actual);
}

static void mode_max_preferred__no_preferred(void **state) {
	const struct WlrMode *actual = wlr_mode_max_preferred(wlr_modes, NULL);

	assert_nul(actual);
}

static void mode_max_preferred__preferred_matches(void **state) {
	struct WlrMode *expected = wlr_mode_init_whr_pref(111, 222, 333);
	pset_add(wlr_modes, expected);

	const struct WlrMode *actual = wlr_mode_max_preferred(wlr_modes, NULL);

	assert_mode_equal(actual, expected);
	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__prior_matches(void **state) {
	struct WlrMode *expected = wlr_mode_init_whr(111, 222, 333);
	pset_add(wlr_modes, expected);

	const struct WlrMode *preferred = wlr_mode_init_whr_pref(111, 222, 333);
	pset_add(wlr_modes, preferred);

	const struct WlrMode *actual = wlr_mode_max_preferred(wlr_modes, NULL);

	assert_mode_equal(actual, expected);
	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__later_higher_refresh(void **state) {
	const struct WlrMode *preferred = wlr_mode_init_whr_pref(111, 222, 333);
	pset_add(wlr_modes, preferred);

	struct WlrMode *expected = wlr_mode_init_whr(111, 222, 999999);
	pset_add(wlr_modes, expected);

	const struct WlrMode *actual = wlr_mode_max_preferred(wlr_modes, NULL);

	assert_mode_equal(actual, expected);
	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__earlier_higher_refresh(void **state) {
	struct WlrMode *expected = wlr_mode_init_whr(111, 222, 999999);
	pset_add(wlr_modes, expected);

	const struct WlrMode *preferred = wlr_mode_init_whr_pref(111, 222, 333);
	pset_add(wlr_modes, preferred);

	const struct WlrMode *actual = wlr_mode_max_preferred(wlr_modes, NULL);

	assert_mode_equal(actual, expected);
	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__failed(void **state) {
	const struct WlrMode *failed = wlr_mode_init_whr(111, 222, 2000);
	pset_add(wlr_modes, failed);
	pset_add(wlr_modes_failed, failed);

	const struct WlrMode *preferred = wlr_mode_init_whr_pref(111, 222, 333);
	pset_add(wlr_modes, preferred);

	struct WlrMode *expected = wlr_mode_init_whr(111, 222, 1000);
	pset_add(wlr_modes, expected);

	const struct WlrMode *actual = wlr_mode_max_preferred(wlr_modes, wlr_modes_failed);

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

