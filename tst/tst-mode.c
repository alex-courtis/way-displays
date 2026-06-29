#include "tst.h"

#include "assert-log.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "cfg/user-mode.h"
#include "fn.h"
#include "head.h"
#include "slist.h"

#include "mode.h"

struct UserMode *user_mode = NULL;
const struct PMap *wlr_modes = NULL;
struct SList *modes_failed = NULL;

static int keys[12] = { 10, 11, 12, 13, 14, 15, 20, 21, 22, 23, 24, 25, };
static void *K00 = &keys[0];
static void *K01 = &keys[1];
static void *K02 = &keys[2];
static void *K03 = &keys[3];
static void *K04 = &keys[4];
static void *K05 = &keys[5];
static void *K20 = &keys[6];
static void *K21 = &keys[7];
static void *K22 = &keys[8];

static int before_each(void **state) {
	assert_logs_empty_before();

	wlr_modes = wlr_mode_pmap_init();
	pmap_put(wlr_modes, K00, wlr_mode_init(NULL, NULL, 200, 100, 59999, false));
	pmap_put(wlr_modes, K01, wlr_mode_init(NULL, NULL, 200, 100, 60499, false));
	pmap_put(wlr_modes, K02, wlr_mode_init(NULL, NULL, 200, 100, 60500, false));
	pmap_put(wlr_modes, K03, wlr_mode_init(NULL, NULL, 400, 200, 120000, false));
	pmap_put(wlr_modes, K04, wlr_mode_init(NULL, NULL, 600, 300, 164999, false));
	pmap_put(wlr_modes, K05, wlr_mode_init(NULL, NULL, 800, 400, 144000, false));

	return 0;
}

static int after_each(void **state) {
	free(user_mode);
	user_mode = NULL;

	slist_free(&modes_failed);
	pmap_free_vals(wlr_modes);

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

	const struct WlrMode *actual = mode_user_mode(wlr_modes, modes_failed, user_mode);

	assert_ptr_equal(actual, pmap_get(wlr_modes, K05));
}

static void mode_user_mode__no_hz_no_match(void **state) {
	user_mode = user_mode_init(false, 999, 999, -1, false);

	const struct WlrMode *actual = mode_user_mode(wlr_modes, modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_user_mode__no_hz_match(void **state) {
	user_mode = user_mode_init(false, 400, 200, -1, false);

	const struct WlrMode *actual = mode_user_mode(wlr_modes, modes_failed, user_mode);

	assert_ptr_equal(actual, pmap_get(wlr_modes, K03));
}

static void mode_user_mode__even_hz_no_match(void **state) {
	user_mode = user_mode_init(false, 200, 100, 144000, false);

	const struct WlrMode *actual = mode_user_mode(wlr_modes, modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_user_mode__even_hz_match(void **state) {
	user_mode = user_mode_init(false, 200, 100, 60000, false);

	const struct WlrMode *actual = mode_user_mode(wlr_modes, modes_failed, user_mode);

	assert_ptr_equal(actual, pmap_get(wlr_modes, K01));
}

static void mode_user_mode__even_hz_rounded_up(void **state) {
	user_mode = user_mode_init(false, 600, 300, 165000, false);

	const struct WlrMode *actual = mode_user_mode(wlr_modes, modes_failed, user_mode);

	assert_ptr_equal(actual, pmap_get(wlr_modes, K04));
}

static void mode_user_mode__failed(void **state) {
	user_mode = user_mode_init(false, 200, 100, 60000, false);

	slist_append(&modes_failed, (void*)pmap_get(wlr_modes, K01));

	const struct WlrMode *actual = mode_user_mode(wlr_modes, modes_failed, user_mode);

	assert_ptr_equal(actual, pmap_get(wlr_modes, K00));
}

static void mode_user_mode__exact_hz_match(void **state) {
	user_mode = user_mode_init(false, 200, 100, 60499, false);

	const struct WlrMode *actual = mode_user_mode(wlr_modes, modes_failed, user_mode);

	assert_ptr_equal(actual, pmap_get(wlr_modes, K01));
}

static void mode_user_mode__exact_hz_failed(void **state) {
	user_mode = user_mode_init(false, 200, 100, 60499, false);

	slist_append(&modes_failed, (void*)pmap_get(wlr_modes, K01));

	const struct WlrMode *actual = mode_user_mode(wlr_modes, modes_failed, user_mode);

	assert_ptr_equal(actual, pmap_get(wlr_modes, K00));
}

static void mode_user_mode__width_failed(void **state) {
	user_mode = user_mode_init(false, 1000, 100, 60499, false);

	const struct WlrMode *actual = mode_user_mode(wlr_modes, modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_user_mode__height_failed(void **state) {
	user_mode = user_mode_init(false, 200, 9999999, 60499, false);

	const struct WlrMode *actual = mode_user_mode(wlr_modes, modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_dpi__(void **state) {
	struct Head head = { .width_mm = 1000, .height_mm = 500, };
	struct WlrMode mode = { .width    = 2000, .height    = 1000, .head = &head };

	// nice roundish number to prevent odd test fails
	double expected = 50.8;

	double actual = mode_dpi(&mode);

	assert_float_equal(actual, expected, 0);
}

static void mode_preferred__no_preferred(void **state) {
	const struct WlrMode *actual = mode_preferred(wlr_modes, NULL);

	assert_nul(actual);
}

static void mode_preferred__preferred(void **state) {
	struct WlrMode *expected = wlr_mode_init(NULL, NULL, 111, 222, 333, true);
	pmap_put(wlr_modes, K20, expected);

	const struct WlrMode *actual = mode_preferred(wlr_modes, NULL);

	assert_ptr_equal(actual, expected);
}

static void mode_preferred__preferred_failed(void **state) {
	struct WlrMode *expected = wlr_mode_init(NULL, NULL, 111, 222, 333, true);
	pmap_put(wlr_modes, K20, expected);

	struct SList *modes_failed = NULL;
	slist_append(&modes_failed, expected);

	const struct WlrMode *actual = mode_preferred(wlr_modes, modes_failed);

	assert_nul(actual);
}

static void mode_max_preferred__no_preferred(void **state) {
	const struct WlrMode *actual = mode_max_preferred(wlr_modes, NULL);

	assert_nul(actual);
}

static void mode_max_preferred__preferred_matches(void **state) {
	struct WlrMode *expected = wlr_mode_init(NULL, NULL, 111, 222, 333, true);
	pmap_put(wlr_modes, K20, expected);

	const struct WlrMode *actual = mode_max_preferred(wlr_modes, NULL);

	assert_non_nul(actual);
	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__prior_matches(void **state) {
	struct WlrMode *expected = wlr_mode_init(NULL, NULL, 111, 222, 333, false);
	pmap_put(wlr_modes, K20, expected);

	struct WlrMode *preferred = wlr_mode_init(NULL, NULL, 111, 222, 333, true);
	pmap_put(wlr_modes, K21, preferred);

	const struct WlrMode *actual = mode_max_preferred(wlr_modes, NULL);

	assert_non_nul(actual);
	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__later_higher_refresh(void **state) {
	struct WlrMode *preferred = wlr_mode_init(NULL, NULL, 111, 222, 333, true);
	pmap_put(wlr_modes, K20, preferred);

	struct WlrMode *expected = wlr_mode_init(NULL, NULL, 111, 222, 999999, false);
	pmap_put(wlr_modes, K21, expected);

	const struct WlrMode *actual = mode_max_preferred(wlr_modes, NULL);

	assert_non_nul(actual);
	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__earlier_higher_refresh(void **state) {
	struct WlrMode *expected = wlr_mode_init(NULL, NULL, 111, 222, 999999, false);
	pmap_put(wlr_modes, K20, expected);

	struct WlrMode *preferred = wlr_mode_init(NULL, NULL, 111, 222, 333, true);
	pmap_put(wlr_modes, K21, preferred);

	const struct WlrMode *actual = mode_max_preferred(wlr_modes, NULL);

	assert_non_nul(actual);
	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__failed(void **state) {
	struct WlrMode *failed = wlr_mode_init(NULL, NULL, 111, 222, 2000, false);
	pmap_put(wlr_modes, K20, failed);
	slist_append(&modes_failed, failed);

	struct WlrMode *preferred = wlr_mode_init(NULL, NULL, 111, 222, 333, true);
	pmap_put(wlr_modes, K21, preferred);

	struct WlrMode *expected = wlr_mode_init(NULL, NULL, 111, 222, 1000, false);
	pmap_put(wlr_modes, K22, expected);

	const struct WlrMode *actual = mode_max_preferred(wlr_modes, modes_failed);

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

