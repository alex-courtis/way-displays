// IWYU pragma: no_include <cmocka.h>
#include "tst.h" // IWYU pragma: keep

#include <stddef.h>
#include <stdio.h>

#include <head.h>
#include <server.h>
#include <list.h>

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
	assert_int_equal(wl_fixed_from_int(1), head_auto_scale(NULL));

	// no desired mode
	assert_int_equal(wl_fixed_from_int(1), head_auto_scale(&head));
}

void head_auto_scale__mode(void **state) {
	struct Mode mode = { 0 };
	struct Head head = { 0 };
	head.desired.mode = &mode;

	// dpi 0 defaults to 96
	expect_value(__wrap_mode_dpi, mode, &mode);
	will_return(__wrap_mode_dpi, 0);
	assert_int_equal(wl_fixed_from_double(96 / 96), head_auto_scale(&head));

	// even 144
	expect_value(__wrap_mode_dpi, mode, &mode);
	will_return(__wrap_mode_dpi, 144);
	assert_int_equal(wl_fixed_from_double(144.0 / 96), head_auto_scale(&head));

	// rounded down 156
	expect_value(__wrap_mode_dpi, mode, &mode);
	will_return(__wrap_mode_dpi, 161);
	assert_int_equal(wl_fixed_from_double(156.0 / 96), head_auto_scale(&head));

	// rounded up 168
	expect_value(__wrap_mode_dpi, mode, &mode);
	will_return(__wrap_mode_dpi, 162);
	assert_int_equal(wl_fixed_from_double(168.0 / 96), head_auto_scale(&head));
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
	};

	return RUN(tests);
}

