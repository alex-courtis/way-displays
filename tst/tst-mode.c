#include "tst.h"

#include "assert-log.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "cfg/user-mode.h"
#include "fn.h"
#include "head.h"
#include "pset.h"
#include "slist.h"

#include "mode.h"

struct UserMode *user_mode = NULL;
const struct PSet *wlr_modes = NULL;
struct SList *wlr_modes_failed = NULL;

struct WlrMode *mode0, *mode1, *mode2, *mode3, *mode4, *mode5;

static int before_each(void **state) {
	assert_logs_empty_before();

	mode0 = wlr_mode_init(NULL, NULL, 200, 100, 59999, false);
	mode1 = wlr_mode_init(NULL, NULL, 200, 100, 60499, false);
	mode2 = wlr_mode_init(NULL, NULL, 200, 100, 60500, false);
	mode3 = wlr_mode_init(NULL, NULL, 400, 200, 120000, false);
	mode4 = wlr_mode_init(NULL, NULL, 600, 300, 164999, false);
	mode5 = wlr_mode_init(NULL, NULL, 800, 400, 144000, false);

	wlr_modes = wlr_mode_pset_init();

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

	slist_free(&wlr_modes_failed);
	pset_free_vals(wlr_modes);

	return 0;
}

static void mode__sort(void **state) {
	struct SList *unsorted = NULL;

	slist_append(&unsorted, wlr_mode_init(NULL, NULL, 1000, 2000, 3000, false)); // 0
	slist_append(&unsorted, wlr_mode_init(NULL, NULL, 1000, 9999, 3000, false)); // 1
	slist_append(&unsorted, wlr_mode_init(NULL, NULL, 1000, 2000, 9999, false)); // 2
	slist_append(&unsorted, wlr_mode_init(NULL, NULL, 9999, 2000, 3000, false)); // 3
	slist_append(&unsorted, wlr_mode_init(NULL, NULL, 1000, 2000, 3000, false)); // 4

	struct SList *sorted = slist_sort(unsorted, (fn_less_than)mode_greater_than_res_refresh);

	assert_ptr_equal(slist_at(sorted, 0), slist_at(unsorted, 3));
	assert_ptr_equal(slist_at(sorted, 1), slist_at(unsorted, 1));
	assert_ptr_equal(slist_at(sorted, 2), slist_at(unsorted, 2));
	assert_ptr_equal(slist_at(sorted, 3), slist_at(unsorted, 0));
	assert_ptr_equal(slist_at(sorted, 4), slist_at(unsorted, 4));

	slist_free_vals(&unsorted, NULL);
	slist_free(&sorted);
}

static void mode_mhz_to_hz_str__(void **state) {
	assert_str_equal(mhz_to_hz_str(0), "0");
	assert_str_equal(mhz_to_hz_str(99000), "99");
	assert_str_equal(mhz_to_hz_str(12300), "12.3");
	assert_str_equal(mhz_to_hz_str(12345), "12.345");
}

static void mode_hz_str_to_mhz__(void **state) {
	assert_int_equal(hz_str_to_mhz(NULL), 0);
	assert_int_equal(hz_str_to_mhz(""), 0);
	assert_int_equal(hz_str_to_mhz("abcde"), 0);
	assert_int_equal(hz_str_to_mhz("12"), 12000);
	assert_int_equal(hz_str_to_mhz("12.34"), 12340);
	assert_int_equal(hz_str_to_mhz("12.3456"), 12346);
}

static void mode_mhz_to_hz_rounded__(void **state) {
	assert_int_equal(mhz_to_hz_rounded(0), 0);
	assert_int_equal(mhz_to_hz_rounded(123567), 124);
}

static void mode_user_mode__max(void **state) {
	user_mode = user_mode_init(true, -1, -1, -1, false);

	const struct WlrMode *actual = mode_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_ptr_equal(actual,  mode5);
}

static void mode_user_mode__no_hz_no_match(void **state) {
	user_mode = user_mode_init(false, 999, 999, -1, false);

	const struct WlrMode *actual = mode_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_user_mode__no_hz_match(void **state) {
	user_mode = user_mode_init(false, 400, 200, -1, false);

	const struct WlrMode *actual = mode_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_ptr_equal(actual, mode3);
}

static void mode_user_mode__even_hz_no_match(void **state) {
	user_mode = user_mode_init(false, 200, 100, 144000, false);

	const struct WlrMode *actual = mode_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_user_mode__even_hz_match(void **state) {
	user_mode = user_mode_init(false, 200, 100, 60000, false);

	const struct WlrMode *actual = mode_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_ptr_equal(actual, mode1);
}

static void mode_user_mode__even_hz_rounded_up(void **state) {
	user_mode = user_mode_init(false, 600, 300, 165000, false);

	const struct WlrMode *actual = mode_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_ptr_equal(actual, mode4);
}

static void mode_user_mode__failed(void **state) {
	user_mode = user_mode_init(false, 200, 100, 60000, false);

	slist_append(&wlr_modes_failed, (void*)mode1);

	const struct WlrMode *actual = mode_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_ptr_equal(actual, mode0);
}

static void mode_user_mode__exact_hz_match(void **state) {
	user_mode = user_mode_init(false, 200, 100, 60499, false);

	const struct WlrMode *actual = mode_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_ptr_equal(actual, mode1);
}

static void mode_user_mode__exact_hz_failed(void **state) {
	user_mode = user_mode_init(false, 200, 100, 60499, false);

	slist_append(&wlr_modes_failed, (void*)mode1);

	const struct WlrMode *actual = mode_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_ptr_equal(actual, mode0);
}

static void mode_user_mode__width_failed(void **state) {
	user_mode = user_mode_init(false, 1000, 100, 60499, false);

	const struct WlrMode *actual = mode_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_user_mode__height_failed(void **state) {
	user_mode = user_mode_init(false, 200, 9999999, 60499, false);

	const struct WlrMode *actual = mode_user_mode(wlr_modes, wlr_modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_dpi__(void **state) {
	struct Head *head = head_init();
	head->width_mm = 1000;
	head->height_mm = 500;

	const struct WlrMode *wlr_mode = wlr_mode_init(head, NULL, 2000, 1000, 0, false);
	pset_add(head->wlr_modes, wlr_mode);

	// nice roundish number to prevent odd test fails
	double expected = 50.8;

	double actual = mode_dpi(wlr_mode);

	assert_float_equal(actual, expected, 0);

	head_free(head);
}

static void mode_preferred__no_preferred(void **state) {
	const struct WlrMode *actual = mode_preferred(wlr_modes, NULL);

	assert_nul(actual);
}

static void mode_preferred__preferred(void **state) {
	struct WlrMode *expected = wlr_mode_init(NULL, NULL, 111, 222, 333, true);
	pset_add(wlr_modes, expected);

	const struct WlrMode *actual = mode_preferred(wlr_modes, NULL);

	assert_ptr_equal(actual, expected);
}

static void mode_preferred__preferred_failed(void **state) {
	struct WlrMode *expected = wlr_mode_init(NULL, NULL, 111, 222, 333, true);
	pset_add(wlr_modes, expected);

	slist_append(&wlr_modes_failed, expected);

	const struct WlrMode *actual = mode_preferred(wlr_modes, wlr_modes_failed);

	assert_nul(actual);
}

static void mode_max_preferred__no_preferred(void **state) {
	const struct WlrMode *actual = mode_max_preferred(wlr_modes, NULL);

	assert_nul(actual);
}

static void mode_max_preferred__preferred_matches(void **state) {
	struct WlrMode *expected = wlr_mode_init(NULL, NULL, 111, 222, 333, true);
	pset_add(wlr_modes, expected);

	const struct WlrMode *actual = mode_max_preferred(wlr_modes, NULL);

	assert_non_nul(actual);
	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__prior_matches(void **state) {
	struct WlrMode *expected = wlr_mode_init(NULL, NULL, 111, 222, 333, false);
	pset_add(wlr_modes, expected);

	const struct WlrMode *preferred = wlr_mode_init(NULL, NULL, 111, 222, 333, true);
	pset_add(wlr_modes, preferred);

	const struct WlrMode *actual = mode_max_preferred(wlr_modes, NULL);

	assert_non_nul(actual);
	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__later_higher_refresh(void **state) {
	const struct WlrMode *preferred = wlr_mode_init(NULL, NULL, 111, 222, 333, true);
	pset_add(wlr_modes, preferred);

	struct WlrMode *expected = wlr_mode_init(NULL, NULL, 111, 222, 999999, false);
	pset_add(wlr_modes, expected);

	const struct WlrMode *actual = mode_max_preferred(wlr_modes, NULL);

	assert_non_nul(actual);
	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__earlier_higher_refresh(void **state) {
	struct WlrMode *expected = wlr_mode_init(NULL, NULL, 111, 222, 999999, false);
	pset_add(wlr_modes, expected);

	const struct WlrMode *preferred = wlr_mode_init(NULL, NULL, 111, 222, 333, true);
	pset_add(wlr_modes, preferred);

	const struct WlrMode *actual = mode_max_preferred(wlr_modes, NULL);

	assert_non_nul(actual);
	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__failed(void **state) {
	struct WlrMode *failed = wlr_mode_init(NULL, NULL, 111, 222, 2000, false);
	pset_add(wlr_modes, failed);
	slist_append(&wlr_modes_failed, failed);

	const struct WlrMode *preferred = wlr_mode_init(NULL, NULL, 111, 222, 333, true);
	pset_add(wlr_modes, preferred);

	struct WlrMode *expected = wlr_mode_init(NULL, NULL, 111, 222, 1000, false);
	pset_add(wlr_modes, expected);

	const struct WlrMode *actual = mode_max_preferred(wlr_modes, wlr_modes_failed);

	assert_non_nul(actual);
	assert_ptr_equal(actual, expected);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(mode__sort),

		TEST_BA(mode_mhz_to_hz_str__),
		TEST_BA(mode_hz_str_to_mhz__),
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

