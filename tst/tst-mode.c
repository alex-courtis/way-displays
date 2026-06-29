#include "tst.h"

#include "assert-log.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "cfg/user-mode.h"
#include "head.h"
#include "slist.h"

#include "mode.h"

struct UserMode *user_mode = NULL;
struct SList *modes = NULL;
struct SList *modes_failed = NULL;

static int before_all(void **state) {
	return 0;
}

static int after_all(void **state) {
	return 0;
}

static int before_each(void **state) {
	assert_logs_empty_before();

	slist_append(&modes, mode_init(NULL, NULL, 200, 100, 59999, false));
	slist_append(&modes, mode_init(NULL, NULL, 200, 100, 60499, false));
	slist_append(&modes, mode_init(NULL, NULL, 200, 100, 60500, false));
	slist_append(&modes, mode_init(NULL, NULL, 400, 200, 120000, false));
	slist_append(&modes, mode_init(NULL, NULL, 600, 300, 164999, false));
	slist_append(&modes, mode_init(NULL, NULL, 800, 400, 144000, false));

	return 0;
}

static int after_each(void **state) {
	free(user_mode);
	user_mode = NULL;

	slist_free_vals(&modes, NULL);
	slist_free(&modes_failed);

	return 0;
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

	struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_ptr_equal(actual, slist_at(modes, 5));
}

static void mode_user_mode__no_hz_no_match(void **state) {
	user_mode = user_mode_init(false, 999, 999, -1, false);

	const struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_user_mode__no_hz_match(void **state) {
	user_mode = user_mode_init(false, 400, 200, -1, false);

	struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_ptr_equal(actual, slist_at(modes, 3));
}

static void mode_user_mode__even_hz_no_match(void **state) {
	user_mode = user_mode_init(false, 200, 100, 144000, false);

	const struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_user_mode__even_hz_match(void **state) {
	user_mode = user_mode_init(false, 200, 100, 60000, false);

	struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_ptr_equal(actual, slist_at(modes, 1));
}

static void mode_user_mode__even_hz_rounded_up(void **state) {
	user_mode = user_mode_init(false, 600, 300, 165000, false);

	struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_ptr_equal(actual, slist_at(modes, 4));
}

static void mode_user_mode__failed(void **state) {
	user_mode = user_mode_init(false, 200, 100, 60000, false);

	slist_append(&modes_failed, slist_at(modes, 1));

	struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_ptr_equal(actual, slist_at(modes, 0));
}

static void mode_user_mode__exact_hz_match(void **state) {
	user_mode = user_mode_init(false, 200, 100, 60499, false);

	struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_ptr_equal(actual, slist_at(modes, 1));
}

static void mode_user_mode__exact_hz_failed(void **state) {
	user_mode = user_mode_init(false, 200, 100, 60499, false);

	slist_append(&modes_failed, slist_at(modes, 1));

	struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_ptr_equal(actual, slist_at(modes, 0));
}

static void mode_dpi__(void **state) {
	struct Head head = { .width_mm = 1000, .height_mm = 500, };
	struct Mode mode = { .width    = 2000, .height    = 1000, .head = &head };

	// nice roundish number to prevent odd test fails
	double expected = 50.8;

	double actual = mode_dpi(&mode);

	assert_float_equal(actual, expected, 0);
}

static void user_mode_equal__max_not_equal(void **state) {
	struct UserMode *a = user_mode_init(true, 1, 1, 1, false);
	struct UserMode *b = user_mode_init(false, 1, 1, 1, false);

	assert_false(user_mode_equal(a, b));

	user_mode_free(a);
	user_mode_free(b);
}

static void user_mode_equal__wh_not_equal(void **state) {
	struct UserMode *a = user_mode_init(false, 10, 1, 1, false);
	struct UserMode *b = user_mode_init(false, 1, 1, 1, false);

	assert_false(user_mode_equal(a, b));

	a->width = b->width;

	assert_true(user_mode_equal(a, b));

	a->height = 20;

	assert_false(user_mode_equal(a, b));

	user_mode_free(a);
	user_mode_free(b);
}

static void user_mode_equal__refresh_not_equal(void **state) {
	struct UserMode *a = user_mode_init(false, 1, 1, 10, false);
	struct UserMode *b = user_mode_init(false, 1, 1, 1, false);

	assert_false(user_mode_equal(a, b));

	a->refresh_mhz = b->refresh_mhz;

	assert_true(user_mode_equal(a, b));

	a->refresh_mhz = -1;

	assert_false(user_mode_equal(a, b));

	a->refresh_mhz = 1;
	b->refresh_mhz = -1;

	assert_false(user_mode_equal(a, b));

	a->refresh_mhz = -1;
	b->refresh_mhz = -1;

	assert_true(user_mode_equal(a, b));

	user_mode_free(a);
	user_mode_free(b);
}

static void mode_preferred__no_preferred(void **state) {
	const struct Mode *actual = mode_preferred(modes, NULL);

	assert_nul(actual);
}

static void mode_preferred__preferred(void **state) {
	struct Mode *expected = mode_init(NULL, NULL, 111, 222, 333, true);
	slist_append(&modes, expected);

	const struct Mode *actual = mode_preferred(modes, NULL);

	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__no_preferred(void **state) {
	const struct Mode *actual = mode_max_preferred(modes, NULL);

	assert_nul(actual);
}

static void mode_max_preferred__preferred_matches(void **state) {
	struct Mode *expected = mode_init(NULL, NULL, 111, 222, 333, true);
	slist_append(&modes, expected);

	struct Mode *actual = mode_max_preferred(modes, NULL);

	assert_non_nul(actual);
	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__prior_matches(void **state) {
	struct Mode *expected = mode_init(NULL, NULL, 111, 222, 333, false);
	slist_append(&modes, expected);

	struct Mode *preferred = mode_init(NULL, NULL, 111, 222, 333, true);
	slist_append(&modes, preferred);

	struct Mode *actual = mode_max_preferred(modes, NULL);

	assert_non_nul(actual);
	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__later_higher_refresh(void **state) {
	struct Mode *preferred = mode_init(NULL, NULL, 111, 222, 333, true);
	slist_append(&modes, preferred);

	struct Mode *expected = mode_init(NULL, NULL, 111, 222, 999999, false);
	slist_append(&modes, expected);

	struct Mode *actual = mode_max_preferred(modes, NULL);

	assert_non_nul(actual);
	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__earlier_higher_refresh(void **state) {
	struct Mode *expected = mode_init(NULL, NULL, 111, 222, 999999, false);
	slist_append(&modes, expected);

	struct Mode *preferred = mode_init(NULL, NULL, 111, 222, 333, true);
	slist_append(&modes, preferred);

	struct Mode *actual = mode_max_preferred(modes, NULL);

	assert_non_nul(actual);
	assert_ptr_equal(actual, expected);
}

static void mode_max_preferred__failed(void **state) {
	struct Mode *failed = mode_init(NULL, NULL, 111, 222, 2000, false);
	slist_append(&modes, failed);
	slist_append(&modes_failed, failed);

	struct Mode *preferred = mode_init(NULL, NULL, 111, 222, 333, true);
	slist_append(&modes, preferred);

	struct Mode *expected = mode_init(NULL, NULL, 111, 222, 1000, false);
	slist_append(&modes, expected);

	struct Mode *actual = mode_max_preferred(modes, modes_failed);

	assert_non_nul(actual);
	assert_ptr_equal(actual, expected);
}

static void greater_than_res_refresh__width(void **state) {
	struct Mode *a = mode_init(NULL, NULL, 111, 222, 2000, false);
	struct Mode *b = mode_init(NULL, NULL, 10000, 222, 2000, false);

	assert_false(mode_greater_than_res_refresh(a, b));
	assert_true(mode_greater_than_res_refresh(b, a));

	mode_free(a);
	mode_free(b);
}

static void greater_than_res_refresh__height(void **state) {
	struct Mode *a = mode_init(NULL, NULL, 111, 222, 2000, false);
	struct Mode *b = mode_init(NULL, NULL, 111, 10000, 2000, false);

	assert_false(mode_greater_than_res_refresh(a, b));
	assert_true(mode_greater_than_res_refresh(b, a));

	mode_free(a);
	mode_free(b);
}

static void greater_than_res_refresh__refresh(void **state) {
	struct Mode *a = mode_init(NULL, NULL, 111, 222, 2000, false);
	struct Mode *b = mode_init(NULL, NULL, 111, 222, 10000, false);

	assert_false(mode_greater_than_res_refresh(a, b));
	assert_true(mode_greater_than_res_refresh(b, a));

	mode_free(a);
	mode_free(b);
}

int main(void) {
	const struct CMUnitTest tests[] = {
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

		TEST_BA(mode_dpi__),

		TEST_BA(user_mode_equal__max_not_equal),
		TEST_BA(user_mode_equal__wh_not_equal),
		TEST_BA(user_mode_equal__refresh_not_equal),

		TEST_BA(mode_preferred__no_preferred),
		TEST_BA(mode_preferred__preferred),

		TEST_BA(mode_max_preferred__no_preferred),
		TEST_BA(mode_max_preferred__preferred_matches),
		TEST_BA(mode_max_preferred__prior_matches),
		TEST_BA(mode_max_preferred__later_higher_refresh),
		TEST_BA(mode_max_preferred__earlier_higher_refresh),
		TEST_BA(mode_max_preferred__failed),

		TEST_BA(greater_than_res_refresh__width),
		TEST_BA(greater_than_res_refresh__height),
		TEST_BA(greater_than_res_refresh__refresh),
	};

	// TODO remove BA after regression testing
	return RUN_BA(tests);
}

