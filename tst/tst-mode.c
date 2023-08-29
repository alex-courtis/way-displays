#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "mode.h"

struct UserMode *user_mode = NULL;
struct SList *modes = NULL;
struct SList *modes_failed = NULL;

int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	slist_append(&modes, mode_init(NULL, NULL, 200, 100, 59999, false));
	slist_append(&modes, mode_init(NULL, NULL, 200, 100, 60499, false));
	slist_append(&modes, mode_init(NULL, NULL, 200, 100, 60500, false));
	slist_append(&modes, mode_init(NULL, NULL, 400, 200, 120, false));
	slist_append(&modes, mode_init(NULL, NULL, 600, 300, 165, false));
	slist_append(&modes, mode_init(NULL, NULL, 800, 400, 144, false));

	return 0;
}

int after_each(void **state) {
	assert_logs_empty();

	cfg_user_mode_free(user_mode);
	user_mode = NULL;

	slist_free_vals(&modes, NULL);
	slist_free(&modes_failed);

	return 0;
}

void mode_mhz_to_hz_str__(void **state) {
	assert_string_equal_nn(mhz_to_hz_str(0), "0");
	assert_string_equal_nn(mhz_to_hz_str(99000), "99");
	assert_string_equal_nn(mhz_to_hz_str(12300), "12.3");
	assert_string_equal_nn(mhz_to_hz_str(12345), "12.345");
}

void mode_hz_str_to_mhz__(void **state) {
	assert_int_equal(hz_str_to_mhz(NULL), 0);
	assert_int_equal(hz_str_to_mhz(""), 0);
	assert_int_equal(hz_str_to_mhz("abcde"), 0);
	assert_int_equal(hz_str_to_mhz("12"), 12000);
	assert_int_equal(hz_str_to_mhz("12.34"), 12340);
	assert_int_equal(hz_str_to_mhz("12.3456"), 12346);
}

void mode_mhz_to_hz_rounded__(void **state) {
	assert_int_equal(mhz_to_hz_rounded(0), 0);
	assert_int_equal(mhz_to_hz_rounded(123567), 124);
}

void mode_user_mode__max(void **state) {
	user_mode = cfg_user_mode_init("um", true, -1, -1, -1, false);

	struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_ptr_equal(actual, slist_at(modes, 5));
}

void mode_user_mode__no_hz_no_match(void **state) {
	user_mode = cfg_user_mode_init("um", false, 999, 999, -1, false);

	struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_null(actual);
}

void mode_user_mode__no_hz_match(void **state) {
	user_mode = cfg_user_mode_init("um", false, 400, 200, -1, false);

	struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_ptr_equal(actual, slist_at(modes, 3));
}

void mode_user_mode__even_hz_no_match(void **state) {
	user_mode = cfg_user_mode_init("um", false, 200, 100, 144, false);

	struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_null(actual);
}

void mode_user_mode__even_hz_match(void **state) {
	user_mode = cfg_user_mode_init("um", false, 200, 100, 60, false);

	struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_ptr_equal(actual, slist_at(modes, 1));
}

void mode_user_mode__failed(void **state) {
	user_mode = cfg_user_mode_init("um", false, 200, 100, 60, false);

	slist_append(&modes_failed, slist_at(modes, 1));

	struct Mode *actual = mode_user_mode(modes, modes_failed, user_mode);

	assert_ptr_equal(actual, slist_at(modes, 0));
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(mode_mhz_to_hz_str__),
		TEST(mode_hz_str_to_mhz__),
		TEST(mode_mhz_to_hz_rounded__),

		TEST(mode_user_mode__max),
		TEST(mode_user_mode__no_hz_no_match),
		TEST(mode_user_mode__no_hz_match),
		TEST(mode_user_mode__even_hz_no_match),
		TEST(mode_user_mode__even_hz_match),
		TEST(mode_user_mode__failed),
	};

	return RUN(tests);
}

