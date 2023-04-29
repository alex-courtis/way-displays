#include "tst.h"
#include "asserts.h"
#include "expects.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "cfg.h"
#include "global.h"
#include "list.h"
#include "mode.h"

#include "head.h"

double __wrap_mode_dpi(struct Mode *mode) {
	check_expected(mode);
	return mock();
}

struct Mode *__wrap_mode_user_mode(struct SList *modes, struct SList *modes_failed, struct UserMode *user_mode) {
	check_expected(modes);
	check_expected(modes_failed);
	check_expected(user_mode);
	return (struct Mode *)mock();
}

struct Mode *__wrap_mode_max_preferred(struct SList *modes, struct SList *modes_failed) {
	check_expected(modes);
	check_expected(modes_failed);
	return (struct Mode *)mock();
}


int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	cfg = cfg_default();
	return 0;
}

int after_each(void **state) {
	cfg_destroy();
	return 0;
}


void head_auto_scale__default(void **state) {
	struct Head head = { 0 };

	// no head
	assert_wl_fixed_t_equal_double(head_auto_scale(NULL), 2);

	// no desired mode
	assert_wl_fixed_t_equal_double(head_auto_scale(&head), 1);
}

void head_auto_scale__mode(void **state) {
	struct Mode mode = { 0 };
	struct Head head = { .desired.mode = &mode };

	// dpi 0 defaults to 96
	expect_value(__wrap_mode_dpi, mode, &mode);
	will_return(__wrap_mode_dpi, 0);
	assert_wl_fixed_t_equal_double(head_auto_scale(&head), 1);

	// even 144
	expect_value(__wrap_mode_dpi, mode, &mode);
	will_return(__wrap_mode_dpi, 144);
	assert_wl_fixed_t_equal_double(head_auto_scale(&head), 144.0 / 96);

	// rounded down to 156
	expect_value(__wrap_mode_dpi, mode, &mode);
	will_return(__wrap_mode_dpi, 161);
	assert_wl_fixed_t_equal_double(head_auto_scale(&head), 156.0 / 96);

	// rounded up to 168
	expect_value(__wrap_mode_dpi, mode, &mode);
	will_return(__wrap_mode_dpi, 162);
	assert_wl_fixed_t_equal_double(head_auto_scale(&head), 168.0 / 96);
}

void head_scaled_dimensions__default(void **state) {
	struct Head head = { .scaled.width = 1, .scaled.height = 1, };

	// no head
	head_scaled_dimensions(NULL);

	// no mode
	head_scaled_dimensions(&head);
	assert_int_equal(head.scaled.width, 1);
	assert_int_equal(head.scaled.height, 1);

	// no scale
	struct Mode mode = { .width = 200, .height = 100, };
	head.desired.mode = &mode;

	head_scaled_dimensions(&head);
	assert_int_equal(head.scaled.width, 1);
	assert_int_equal(head.scaled.height, 1);
}

void head_scaled_dimensions__calculated(void **state) {
	struct Mode mode = { .width = 200, .height = 100, };
	struct Head head = { .desired.mode = &mode, };

	// double, not rotated
	head.desired.scale = wl_fixed_from_double(0.5);
	head.transform = WL_OUTPUT_TRANSFORM_NORMAL;

	head_scaled_dimensions(&head);
	assert_int_equal(head.scaled.width, 400);
	assert_int_equal(head.scaled.height, 200);

	// one third, rotated
	head.desired.scale = wl_fixed_from_double(3);
	head.transform = WL_OUTPUT_TRANSFORM_90;

	head_scaled_dimensions(&head);
	assert_int_equal(head.scaled.width, 33);
	assert_int_equal(head.scaled.height, 67);
}

void head_find_mode__none(void **state) {
	struct Head head = { 0 };
	struct Mode mode = { 0 };

	// no head
	assert_null(head_find_mode(NULL));

	// all modes failed
	slist_append(&head.modes, &mode);
	slist_append(&head.modes_failed, &mode);
	assert_null(head_find_mode(&head));

	slist_free(&head.modes);
	slist_free(&head.modes_failed);
}

void head_find_mode__user_available(void **state) {
	struct Head head = { 0 };
	struct Mode mode = { 0 };
	slist_append(&head.modes, &mode);

	// user preferred head
	struct UserMode *user_mode = cfg_user_mode_default();
	user_mode->name_desc = strdup("!.*EAD");
	slist_append(&cfg->user_modes, user_mode);
	head.name = strdup("HEAD");

	// mode matched to user
	struct Mode expected = { 0 };
	expect_value(__wrap_mode_user_mode, modes, head.modes);
	expect_value(__wrap_mode_user_mode, modes_failed, head.modes_failed);
	expect_value(__wrap_mode_user_mode, user_mode, user_mode);
	will_return(__wrap_mode_user_mode, &expected);

	assert_ptr_equal(head_find_mode(&head), &expected);

	slist_free(&head.modes);
	free(head.name);
}

void head_find_mode__user_failed(void **state) {
	struct Head head = { 0 };
	struct Mode mode = { 0 };
	slist_append(&head.modes, &mode);

	// user preferred head
	struct UserMode *user_mode = cfg_user_mode_default();
	user_mode->name_desc = strdup("!HEA.*");
	slist_append(&cfg->user_modes, user_mode);
	head.name = strdup("HEAD");

	// mode not matched to user
	expect_value(__wrap_mode_user_mode, modes, head.modes);
	expect_value(__wrap_mode_user_mode, modes_failed, head.modes_failed);
	expect_value(__wrap_mode_user_mode, user_mode, user_mode);
	will_return(__wrap_mode_user_mode, NULL);

	// one and only notices: falling back to preferred then max
	expect_log_warn("\n%s: No available mode for %s, falling back to preferred", "HEAD", "-1x-1", NULL, NULL);
	expect_log_info("\n%s: No preferred mode, falling back to maximum available", "HEAD", NULL, NULL, NULL);

	// user failed, fall back to max
	assert_ptr_equal(head_find_mode(&head), &mode);

	// try a second time
	expect_value(__wrap_mode_user_mode, modes, head.modes);
	expect_value(__wrap_mode_user_mode, modes_failed, head.modes_failed);
	expect_value(__wrap_mode_user_mode, user_mode, user_mode);
	will_return(__wrap_mode_user_mode, NULL);

	// no notices this time
	assert_ptr_equal(head_find_mode(&head), &mode);

	slist_free(&head.modes);
	free(head.name);
}

void head_find_mode__preferred(void **state) {
	struct Head head = { .name = "name", };
	struct Mode mode = { .preferred = true, };

	slist_append(&head.modes, &mode);

	assert_ptr_equal(head_find_mode(&head), &mode);

	slist_free(&head.modes);
}

void head_find_mode__max_preferred_refresh(void **state) {
	struct Head head = { .name = "name", };
	struct Mode mode = { 0 };

	slist_append(&cfg->max_preferred_refresh_name_desc, strdup("!nam.*"));

	slist_append(&head.modes, &mode);

	expect_value(__wrap_mode_max_preferred, modes, head.modes);
	expect_value(__wrap_mode_max_preferred, modes_failed, head.modes_failed);
	will_return(__wrap_mode_max_preferred, &mode);

	assert_ptr_equal(head_find_mode(&head), &mode);

	slist_free(&head.modes);
}

void head_find_mode__max(void **state) {
	struct Head head = { .name = "name", };
	struct Mode mode = { 0 };

	slist_append(&head.modes, &mode);

	// one and only notice
	expect_log_info("\n%s: No preferred mode, falling back to maximum available", "name", NULL, NULL, NULL);

	assert_ptr_equal(head_find_mode(&head), &mode);

	// no notice
	assert_ptr_equal(head_find_mode(&head), &mode);

	slist_free(&head.modes);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(head_auto_scale__default),
		TEST(head_auto_scale__mode),

		TEST(head_scaled_dimensions__default),
		TEST(head_scaled_dimensions__calculated),

		TEST(head_find_mode__none),
		TEST(head_find_mode__user_available),
		TEST(head_find_mode__user_failed),
		TEST(head_find_mode__preferred),
		TEST(head_find_mode__max_preferred_refresh),
		TEST(head_find_mode__max),
	};

	return RUN(tests);
}

