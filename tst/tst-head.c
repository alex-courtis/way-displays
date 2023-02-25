#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <string.h>

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

void __wrap_log_info(const char *__restrict __format, ...) {
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

void head_matches_name_desc_exact__(void **state) {
	struct Head head = { .name = "NN", .description = "DD", };

	assert_true(head_matches_name_desc_exact("NN", &head));
	assert_false(head_matches_name_desc_exact("N", &head));
	assert_false(head_matches_name_desc_exact("NNN", &head));

	assert_true(head_matches_name_desc_exact("DD", &head));
	assert_false(head_matches_name_desc_exact("D", &head));
	assert_false(head_matches_name_desc_exact("DDD", &head));

	assert_false(head_matches_name_desc_exact("ZZ", &head));
}

void head_matches_name_desc_partial__(void **state) {
	struct Head head = { .name = "NN", .description = "DD", };

	assert_true(head_matches_name_desc_partial("N", &head));
	assert_true(head_matches_name_desc_partial("NN", &head));
	assert_false(head_matches_name_desc_partial("NNN", &head));

	assert_true(head_matches_name_desc_partial("D", &head));
	assert_true(head_matches_name_desc_partial("DD", &head));
	assert_false(head_matches_name_desc_partial("DDD", &head));

	assert_false(head_matches_name_desc_partial("Z", &head));
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

		TEST(head_matches_name_desc_exact__),
		TEST(head_matches_name_desc_partial__),

		TEST(head_auto_scale__default),
		TEST(head_auto_scale__mode),

		TEST(head_scaled_dimensions__default),
		TEST(head_scaled_dimensions__calculated),

		TEST(head_find_mode__none),
		TEST(head_find_mode__max),
	};

	return RUN(tests);
}

