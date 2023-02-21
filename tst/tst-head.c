// IWYU pragma: no_include <cmocka.h>
#include "tst.h" // IWYU pragma: keep

#include <stddef.h>
#include <stdio.h>

#include <head.h>
#include <server.h>
#include <list.h>

int setup_all(void **state) {
	return 0;
}

int teardown_all(void **state) {
	return 0;
}

int setup_each(void **state) {
	cfg = cfg_default();
	return 0;
}

int teardown_each(void **state) {
	cfg_destroy();
	return 0;
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
	assert_true(head_matches_name_desc_exact("DD", &head));

	assert_false(head_matches_name_desc_exact("ZZ", &head));
}

void head_matches_name_desc_partial__(void **state) {
	struct Head head = { .name = "NNNN", .description = "DDDD", };

	assert_true(head_matches_name_desc_partial("NN", &head));
	assert_true(head_matches_name_desc_partial("NNNN", &head));
	assert_false(head_matches_name_desc_partial("__NN__", &head));

	assert_true(head_matches_name_desc_partial("DD", &head));
	assert_true(head_matches_name_desc_partial("DDDD", &head));
	assert_false(head_matches_name_desc_partial("--DD--", &head));
}

void head_auto_scale__absent(void **state) {
	struct Head head = { 0 };

	// no head
	assert_int_equal(wl_fixed_from_int(1), head_auto_scale(NULL));

	// no desired mode
	assert_int_equal(wl_fixed_from_int(1), head_auto_scale(&head));

	// TODO mock mode_dpi here
}

int main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(head_is_max_preferred_refresh__nohead, setup_each, teardown_each),
		cmocka_unit_test_setup_teardown(head_is_max_preferred_refresh__match, setup_each, teardown_each),
		cmocka_unit_test_setup_teardown(head_is_max_preferred_refresh__nomatch, setup_each, teardown_each),

		cmocka_unit_test(head_matches_name_desc_exact__),
		cmocka_unit_test(head_matches_name_desc_partial__),

		cmocka_unit_test(head_auto_scale__absent),
	};

	return cmocka_run_group_tests(tests, setup_all, teardown_all);
}

