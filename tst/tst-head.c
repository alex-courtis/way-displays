#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "cfg.h"
#include "head.h"
#include "list.h"
#include "mode.h"
#include "server.h"

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

void __wrap_log_info(const char *__restrict __format, ...) {
	check_expected(__format);
}

void __wrap_log_warn(const char *__restrict __format, ...) {
	check_expected(__format);
}


void head_is_max_preferred_refresh__nohead(void **state) {
	assert_false(head_is_max_preferred_refresh(NULL));
}

void head_is_max_preferred_refresh__match(void **state) {
	slist_append(&cfg->max_preferred_refresh_name_desc, strdup("AA"));

	struct Head head = { .name = "AA", };

	assert_true(head_is_max_preferred_refresh(&head));
}

void head_is_max_preferred_refresh__nomatch(void **state) {
	slist_append(&cfg->max_preferred_refresh_name_desc, strdup("AA"));

	struct Head head = { .name = "ZZ", };

	assert_false(head_is_max_preferred_refresh(&head));
}

void head_auto_scale__default(void **state) {
	struct Head head = { 0 };

	// no head
	assert_wl_fixed_t_equal_double(head_auto_scale(NULL), 1);

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
}

void head_find_mode__user_available(void **state) {
	struct Head head = { 0 };
	struct Mode mode = { 0 };
	slist_append(&head.modes, &mode);

	// user preferred head
	struct UserMode *user_mode = (struct UserMode*)calloc(1, sizeof(struct UserMode));
	user_mode->name_desc = strdup("HEAD");
	slist_append(&cfg->user_modes, user_mode);
	head.name = strdup("HEAD");

	// mode matched to user
	struct Mode expected = { 0 };
	expect_value(__wrap_mode_user_mode, modes, head.modes);
	expect_value(__wrap_mode_user_mode, modes_failed, head.modes_failed);
	expect_value(__wrap_mode_user_mode, user_mode, user_mode);
	will_return(__wrap_mode_user_mode, &expected);

	assert_ptr_equal(head_find_mode(&head), &expected);
}

void head_find_mode__user_failed(void **state) {
	struct Head head = { 0 };
	struct Mode mode = { 0 };
	slist_append(&head.modes, &mode);

	// user preferred head
	struct UserMode *user_mode = (struct UserMode*)calloc(1, sizeof(struct UserMode));
	user_mode->name_desc = strdup("HEAD");
	slist_append(&cfg->user_modes, user_mode);
	head.name = strdup("HEAD");

	// mode not matched to user
	expect_value(__wrap_mode_user_mode, modes, head.modes);
	expect_value(__wrap_mode_user_mode, modes_failed, head.modes_failed);
	expect_value(__wrap_mode_user_mode, user_mode, user_mode);
	will_return(__wrap_mode_user_mode, NULL);

	// one and only notices: falling back to preferred then max
	expect_any(__wrap_log_warn, __format);
	expect_any(__wrap_log_info, __format);

	// user failed, fall back to max
	assert_ptr_equal(head_find_mode(&head), &mode);

	// try a second time
	expect_value(__wrap_mode_user_mode, modes, head.modes);
	expect_value(__wrap_mode_user_mode, modes_failed, head.modes_failed);
	expect_value(__wrap_mode_user_mode, user_mode, user_mode);
	will_return(__wrap_mode_user_mode, NULL);

	// no notices this time
	assert_ptr_equal(head_find_mode(&head), &mode);
}

void head_find_mode__max(void **state) {
	struct Head head = { 0 };
	struct Mode mode = { 0 };

	slist_append(&head.modes, &mode);

	// one and only notice
	expect_any(__wrap_log_info, __format);
	assert_ptr_equal(head_find_mode(&head), &mode);

	// no notice
	assert_ptr_equal(head_find_mode(&head), &mode);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(head_is_max_preferred_refresh__nohead),
		TEST(head_is_max_preferred_refresh__match),
		TEST(head_is_max_preferred_refresh__nomatch),

		TEST(head_auto_scale__default),
		TEST(head_auto_scale__mode),

		TEST(head_scaled_dimensions__default),
		TEST(head_scaled_dimensions__calculated),

		TEST(head_find_mode__none),
		TEST(head_find_mode__user_available),
		TEST(head_find_mode__user_failed),
		TEST(head_find_mode__max),
	};

	return RUN(tests);
}

