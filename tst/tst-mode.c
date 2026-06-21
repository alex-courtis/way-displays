#include "tst.h"

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

static int after_each(void **state) {
	user_mode_free(user_mode);
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
	user_mode = user_mode_init("um", true, -1, -1, -1, false);

	struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_ptr_equal(actual, slist_at(modes, 5));
}

static void mode_user_mode__no_hz_no_match(void **state) {
	user_mode = user_mode_init("um", false, 999, 999, -1, false);

	const struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_user_mode__no_hz_match(void **state) {
	user_mode = user_mode_init("um", false, 400, 200, -1, false);

	struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_ptr_equal(actual, slist_at(modes, 3));
}

static void mode_user_mode__even_hz_no_match(void **state) {
	user_mode = user_mode_init("um", false, 200, 100, 144000, false);

	const struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_nul(actual);
}

static void mode_user_mode__even_hz_match(void **state) {
	user_mode = user_mode_init("um", false, 200, 100, 60000, false);

	struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_ptr_equal(actual, slist_at(modes, 1));
}

static void mode_user_mode__even_hz_rounded_up(void **state) {
	user_mode = user_mode_init("um", false, 600, 300, 165000, false);

	struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_ptr_equal(actual, slist_at(modes, 4));
}

static void mode_user_mode__failed(void **state) {
	user_mode = user_mode_init("um", false, 200, 100, 60000, false);

	slist_append(&modes_failed, slist_at(modes, 1));

	struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_ptr_equal(actual, slist_at(modes, 0));
}

static void mode_user_mode__exact_hz_match(void **state) {
	user_mode = user_mode_init("um", false, 200, 100, 60499, false);

	struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_ptr_equal(actual, slist_at(modes, 1));
}

static void mode_user_mode__exact_hz_failed(void **state) {
	user_mode = user_mode_init("um", false, 200, 100, 60499, false);

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

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_A(mode_mhz_to_hz_str__),
		TEST_A(mode_hz_str_to_mhz__),
		TEST_A(mode_mhz_to_hz_rounded__),

		TEST_A(mode_user_mode__max),
		TEST_A(mode_user_mode__no_hz_no_match),
		TEST_A(mode_user_mode__no_hz_match),
		TEST_A(mode_user_mode__even_hz_no_match),
		TEST_A(mode_user_mode__even_hz_match),
		TEST_A(mode_user_mode__even_hz_rounded_up),
		TEST_A(mode_user_mode__failed),
		TEST_A(mode_user_mode__exact_hz_match),
		TEST_A(mode_user_mode__exact_hz_failed),

		TEST_A(mode_dpi__),
	};

	return RUN(tests);
}

