#include "tst.h"

#include "assert-log.h"
#include "assert-mode.h"
#include "assert-pset.h"
#include "assert-wl.h"
#include "asserts.h"
#include "expects.h"
#include "util-col.h"
#include "util-file.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cfg.h"
#include "fn.h"
#include "log.h"
#include "mode.h"
#include "pset.h"
#include "pslist.h"
#include "spmap.h"
#include "sset.h"

#include "head.h"

static int before_each(void **state) {
	g_cfg = cfg_default();
	return 0;
}

static int after_each(void **state) {
	assert_logs_empty();

	g_cfg_destroy();
	return 0;
}

static void head_get_fixed_scale__rounding_nearest(void **state) {
	g_cfg->scale_round_strategy = NEAREST;

	g_cfg->scale_round_to = 8;
	assert_wl_fixed_t_equal_double(head_get_fixed_scale(1.37), 1.375);

	g_cfg->scale_round_to = 4;
	assert_wl_fixed_t_equal_double(head_get_fixed_scale(1.37), 1.25);

	g_cfg->scale_round_to = 2;
	assert_wl_fixed_t_equal_double(head_get_fixed_scale(1.37), 1.5);

	g_cfg->scale_round_to = 1;
	assert_wl_fixed_t_equal_double(head_get_fixed_scale(1.37), 1);

	// no rounding
	g_cfg->scale_round_to = 8;
	assert_wl_fixed_t_equal_double(head_get_fixed_scale(1.125), 1.125);
}

static void head_get_fixed_scale__rounding_up(void **state) {
	g_cfg->scale_round_strategy = UP;

	g_cfg->scale_round_to = 8;
	assert_wl_fixed_t_equal_double(head_get_fixed_scale(1.37), 1.375);

	g_cfg->scale_round_to = 4;
	assert_wl_fixed_t_equal_double(head_get_fixed_scale(1.37), 1.5);

	g_cfg->scale_round_to = 2;
	assert_wl_fixed_t_equal_double(head_get_fixed_scale(1.37), 1.5);

	g_cfg->scale_round_to = 1;
	assert_wl_fixed_t_equal_double(head_get_fixed_scale(1.37), 2);

	// no rounding
	g_cfg->scale_round_to = 8;
	assert_wl_fixed_t_equal_double(head_get_fixed_scale(1.125), 1.125);
}

static void head_get_fixed_scale__rounding_down(void **state) {
	g_cfg->scale_round_strategy = DOWN;

	g_cfg->scale_round_to = 8;
	assert_wl_fixed_t_equal_double(head_get_fixed_scale(1.37), 1.25);

	g_cfg->scale_round_to = 4;
	assert_wl_fixed_t_equal_double(head_get_fixed_scale(1.37), 1.25);

	g_cfg->scale_round_to = 2;
	assert_wl_fixed_t_equal_double(head_get_fixed_scale(1.37), 1);

	g_cfg->scale_round_to = 1;
	assert_wl_fixed_t_equal_double(head_get_fixed_scale(1.37), 1);
}

static void head_auto_scale__default(void **state) {
	struct Head *head = head_init();

	// no head
	assert_wl_fixed_t_equal_double(head_auto_scale(NULL, 1.0f, -1.0f), 1);

	// no desired mode
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, -1.0f), 1);

	head_free(head);
}

static void head_auto_scale__mode(void **state) {
	struct Head *head = head_init();

	struct Mode *mode = mode_h(head);
	head->desired.mode = mode;
	pset_add(head->modes, mode);

	// dpi 0 defaults to 96
	expect_ptr(__wrap_mode_dpi, mode, mode);
	will_return_int(__wrap_mode_dpi, 0);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, -1.0f), 1);

	// even 144
	expect_ptr(__wrap_mode_dpi, mode, mode);
	will_return_int(__wrap_mode_dpi, 144);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, -1.0f), 144.0 / 96);

	// rounded down to 156
	expect_ptr(__wrap_mode_dpi, mode, mode);
	will_return_int(__wrap_mode_dpi, 161);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, -1.0f), 156.0 / 96);

	// rounded up to 168
	expect_ptr(__wrap_mode_dpi, mode, mode);
	will_return_int(__wrap_mode_dpi, 162);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, -1.0f), 168.0 / 96);

	head_free(head);
}

static void head_auto_scale__range(void **state) {
	struct Head *head = head_init();

	struct Mode *mode = mode_h(head);
	head->desired.mode = mode;
	pset_add(head->modes, mode);

	// scale under 1.0 is clamped to 1.0 with default settings
	expect_ptr(__wrap_mode_dpi, mode, mode);
	will_return_int(__wrap_mode_dpi, 72);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, -1.0f), 1);

	// clamping to some other minimum value works too
	expect_ptr(__wrap_mode_dpi, mode, mode);
	will_return_int(__wrap_mode_dpi, 12);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 0.125f, -1.0f), 0.125f);

	// the minimum value is always positive (quantized to 1/8)
	expect_ptr(__wrap_mode_dpi, mode, mode);
	will_return_int(__wrap_mode_dpi, 1);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, -1.0f, -1.0f), 0.125f);

	// clamping to maximum value works
	expect_ptr(__wrap_mode_dpi, mode, mode);
	will_return_int(__wrap_mode_dpi, 384);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, 2.5f), 2.5f);

	// maximum values under 1.0 are ignored
	expect_ptr(__wrap_mode_dpi, mode, mode);
	will_return_int(__wrap_mode_dpi, 384);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, 0.9f), 4.0f);

	// the configured maximum is respected even with quantization
	expect_ptr(__wrap_mode_dpi, mode, mode);
	will_return_int(__wrap_mode_dpi, 384);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 0.63f, 1.49f), 1.375f);

	// the configured minimum is respected even with quantization
	expect_ptr(__wrap_mode_dpi, mode, mode);
	will_return_int(__wrap_mode_dpi, 12);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 0.63f, -1.0f), 0.75f);

	head_free(head);
}

static void head_find_mode__no_modes(void **state) {
	struct Head *head = head_n("head0");

	expect_int_value(__wrap_callback, t, ERROR);
	expect_str(__wrap_callback, msg1, "head0");
	expect_str(__wrap_callback, msg2, "\n  No mode, disabling");

	assert_nul(head_find_mode(head));

	assert_log(ERROR, "\nNo mode for head0, disabling.\n");

	head_free(head);
}

static void head_find_mode__all_failed(void **state) {
	struct Head *head = head_n("head0");
	const struct Mode *mode = mode_h(head);

	// all modes failed
	pset_add(head->modes_failed, mode);

	expect_int_value(__wrap_callback, t, ERROR);
	expect_str(__wrap_callback, msg1, "head0");
	expect_str(__wrap_callback, msg2, "\n  No mode, disabling");

	assert_nul(head_find_mode(head));

	assert_log(ERROR, "\nNo mode for head0, disabling.\n");

	head_free(head);
}

static void head_find_mode__user_available(void **state) {
	struct Head *head = head_init();
	const struct Mode *mode = mode_h_whr(head, 1, 2, 3);
	pset_add(head->modes, mode);

	// user preferred head
	struct Mode *mode_target = mode_init();
	spmap_put(g_cfg->modes, "!.*EAD", mode_target);
	head->name = strdup("HEAD");

	// mode matched to user
	const struct Mode *expected = mode_h_whr(head, 4, 5, 6);
	pset_add(head->modes, expected);

	expect_ptr(__wrap_mode_best_satisfying, mode_target, mode_target);
	expect_ptr(__wrap_mode_best_satisfying, modes, head->modes);
	will_return_ptr_type(__wrap_mode_best_satisfying, expected, struct Mode*);

	const struct Mode *actual = head_find_mode(head);

	assert_mode_equal(actual, expected);
	assert_ptr_equal(actual, expected);

	head_free(head);
}

static void head_find_mode__user_failed(void **state) {
	struct Head *head = head_init();
	struct Mode *expected = mode_h(head);
	pset_add(head->modes, expected);

	// user preferred head
	struct Mode *mode_target = mode_init();
	spmap_put(g_cfg->modes, "!HEA.*", mode_target);
	head->name = strdup("HEAD");

	// mode not matched to user
	expect_ptr(__wrap_mode_best_satisfying, mode_target, mode_target);
	expect_ptr(__wrap_mode_best_satisfying, modes, head->modes);
	will_return_ptr_type(__wrap_mode_best_satisfying, NULL, struct Mode*);

	expect_int_value(__wrap_callback, t, WARNING);
	expect_str(__wrap_callback, msg1, "HEAD\n  No available mode for user MODE -1x-1, falling back to preferred");
	expect_str(__wrap_callback, msg2, NULL);

	expect_int_value(__wrap_callback, t, WARNING);
	expect_str(__wrap_callback, msg1, "HEAD\n  No preferred mode, falling back to maximum available");
	expect_str(__wrap_callback, msg2, NULL);

	// user failed, fall back to max
	const struct Mode *actual = head_find_mode(head);

	assert_mode_equal(actual, expected);
	assert_ptr_equal(actual, expected);

	// one and only notices: falling back to preferred then max
	assert_log(WARNING, "\nHEAD: No available mode for user MODE -1x-1, falling back to preferred\n");
	assert_log(INFO, "\nHEAD: No preferred mode, falling back to maximum available\n");

	// same test again
	expect_ptr(__wrap_mode_best_satisfying, mode_target, mode_target);
	expect_ptr(__wrap_mode_best_satisfying, modes, head->modes);
	will_return_ptr_type(__wrap_mode_best_satisfying, NULL, struct Mode*);

	// marked failures avoided
	actual = head_find_mode(head);

	assert_mode_equal(actual, expected);
	assert_ptr_equal(actual, expected);

	// no notices this time

	head_free(head);
}

static void head_find_mode__preferred(void **state) {
	struct Head *head = head_n("name");
	struct Mode *expected = mode_h(head);
	head->mode_preferred = expected;

	pset_add(head->modes, expected);

	const struct Mode *actual = head_find_mode(head);

	assert_mode_equal(actual, expected);
	assert_ptr_equal(actual, expected);

	head_free(head);
}

static void head_find_mode__mode_max_refresh(void **state) {
	struct Head *head = head_n("name");
	struct Mode *expected = mode_h(head);
	head->mode_preferred = expected;

	sset_add(g_cfg->max_preferred_refresh, "!nam.*");

	pset_add(head->modes, expected);

	expect_ptr(__wrap_mode_max_refresh, mode_target, expected);
	expect_ptr(__wrap_mode_max_refresh, modes, head->modes);
	will_return_ptr_type(__wrap_mode_max_refresh, expected, struct Mode*);

	const struct Mode *actual = head_find_mode(head);

	assert_mode_equal(actual, expected);
	assert_ptr_equal(actual, expected);

	head_free(head);
}

static void head_find_mode__max(void **state) {
	struct Head *head = head_n("name");

	struct Mode *expected = mode_h(head);
	pset_add(head->modes, expected);

	expect_int_value(__wrap_callback, t, WARNING);
	expect_str(__wrap_callback, msg1, "name\n  No preferred mode, falling back to maximum available");
	expect_str(__wrap_callback, msg2, NULL);

	// one and only notice
	const struct Mode *actual = head_find_mode(head);

	assert_mode_equal(actual, expected);
	assert_ptr_equal(actual, expected);

	assert_log(INFO, "\nname: No preferred mode, falling back to maximum available\n");

	// no notice
	actual = head_find_mode(head);

	assert_mode_equal(actual, expected);
	assert_ptr_equal(actual, expected);

	head_free(head);
}

static void head_find_mode__none(void **state) {
	struct Head *head = head_n("head0");

	expect_int_value(__wrap_callback, t, ERROR);
	expect_str(__wrap_callback, msg1, "head0");
	expect_str(__wrap_callback, msg2, "\n  No mode, disabling");

	assert_nul(head_find_mode(head));

	assert_log(ERROR, "\nNo mode for head0, disabling.\n");

	head_free(head);
}

static void head_max_mode__max(void **state) {
	struct Head *head = head_init();

	struct Mode *mode_expected = mode_whr(2000, 2000, 2000);

	pset_add_many(head->modes,
			mode_whr(1000, 1000, 1000),
			mode_whr(500, 500, 1000),
			mode_whr(1000, 1000, 500),
			mode_whr(2000, 2000, 1000),
			mode_expected,
			mode_whr(1000, 1000, 1000),
			NULL);

	const struct Mode *actual = head_max_mode(head);

	assert_mode_equal(actual, mode_expected);
	assert_ptr_equal(actual, mode_expected);

	head_free(head);
}

static void head_matches_name_desc_regex__name(void **state) {
	struct Head *head = head_n("name");

	assert_true(head_matches_name_desc_regex(head, "!nam"));

	head_free(head);
}

static void head_matches_name_desc_regex__desc(void **state) {
	struct Head *head = head_n("name");
	head->description = strdup("desc");

	assert_true(head_matches_name_desc_regex(head, "!esc"));

	head_free(head);
}

static void head_matches_name_desc_regex__bad(void **state) {
	struct Head *head = head_n("name");

	assert_false(head_matches_name_desc_regex(head, "!(badregex"));

	assert_log(DEBUG, "Could not compile Head NAME_DESC regex '(badregex': Unmatched ( or \\(\n");

	head_free(head);
}

static void head_apply_toggles__none(void **state) {
	struct Head *head = head_n("head0");
	struct Cfg *cfg = cfg_init();

	head_apply_toggles(head, cfg);

	assert_true(head->overrided_enabled == NoOverride);

	cfg_free(cfg);

	head_free(head);
}

static void head_apply_toggles__disabled__enable(void **state) {
	struct Head *head = head_n("head0");
	head->current.enabled = false;
	struct Cfg *cfg = cfg_init();
	pset_add(cfg->disableds, disabled_nd("head0"));

	head_apply_toggles(head, cfg);

	assert_true(head->overrided_enabled == OverrideTrue);
	assert_log(INFO, "\nApplying \"DISABLED\" override for head0\n");

	head_apply_toggles(head, cfg);

	assert_true(head->overrided_enabled == NoOverride);
	assert_log(INFO, "\nResetting \"DISABLED\" override for head0\n");

	cfg_free(cfg);
	head_free(head);
}

static void head_apply_toggles__disabled__disable(void **state) {
	struct Head *head = head_n("head0");
	head->current.enabled = true;
	struct Cfg *cfg = cfg_init();
	pset_add(cfg->disableds, disabled_nd("head0"));

	head_apply_toggles(head, cfg);

	assert_true(head->overrided_enabled == OverrideFalse);
	assert_log(INFO, "\nApplying \"DISABLED\" override for head0\n");

	head_apply_toggles(head, cfg);

	assert_true(head->overrided_enabled == NoOverride);
	assert_log(INFO, "\nResetting \"DISABLED\" override for head0\n");

	cfg_free(cfg);
	head_free(head);
}

static void head_set_description__nulls(void **state) {
	struct Head *head = head_d("orig");

	head_set_description(head, "(null) (null) (null) foo (null) bar baz");

	assert_str_equal(head->description, "foo (null) bar baz");

	head_free(head);
}

static void head_set_description__no_nulls(void **state) {
	struct Head *head = head_d("orig");

	head_set_description(head, "foo");

	assert_str_equal(head->description, "foo");

	head_free(head);
}

static void head_set_description__empty(void **state) {
	struct Head *head = head_d("orig");

	head_set_description(head, "");

	assert_str_equal(head->description, "");

	head_free(head);
}

static void head_set_description__null_input(void **state) {
	struct Head *head = head_d("orig");

	head_set_description(head, NULL);

	assert_nul(head->description);

	head_free(head);
}

static void heads_reapply__(void **state) {
	struct Pslist *heads = NULL;

	struct Head *head_disabled = head_n("DP-7");
	head_disabled->current.enabled = false;

	head_disabled->mode_preferred = mode_h_whr(head_disabled, 3440, 1440, 59999);

	const struct Pset *modes_once_failed = mode_pset_ptr_init();
	pset_add_many(modes_once_failed,
			head_disabled->mode_preferred,
			mode_h_whr(head_disabled, 3840, 2160, 30000),
			mode_h_whr(head_disabled, 3840, 2160, 29970),
			NULL);

	pset_free(head_disabled->modes_failed);
	head_disabled->modes_failed = pset_clone(modes_once_failed);

	pslist_append(&heads, head_disabled);


	struct Head *head_enabled = head_n("eDP-1");
	head_enabled->current.enabled = true;

	head_enabled->current.mode = mode_h_whr(head_enabled, 2256, 1504, 59999);
	head_enabled->mode_preferred = head_enabled->current.mode;
	pset_add(head_enabled->modes, head_enabled->current.mode);

	pslist_append(&heads, head_enabled);


	heads_reapply(heads);


	assert_pset_equal(head_disabled->modes, modes_once_failed);
	assert_int_equal(pset_size(head_disabled->modes_failed), 0);

	char *expected_log = read_file("tst/head/reapply.log");
	assert_log(INFO, expected_log);
	free(expected_log);

	pslist_free_vals(&heads, (fn_free)head_free);
	pset_free(modes_once_failed);
}

static void head_release_mode__nulls(void **state) {
	head_release_mode(NULL);

	struct Mode *mode_releasing = mode_whr(0, 0, 0);

	head_release_mode(mode_releasing);
}

static void head_release_mode__other(void **state) {
	struct Head *head = head_init();

	struct Mode *mode_releasing = mode_h_whr(head, 10, 20, 0);
	pset_add(head->modes, mode_releasing);

	struct Mode *mode_current = mode_h_whr(head, 30, 40, 0);
	pset_add(head->modes, mode_current);
	head->current.mode = mode_current;

	struct Mode *mode_desired = mode_h_whr(head, 50, 60, 0);
	pset_add(head->modes, mode_desired);
	head->desired.mode = mode_desired;

	assert_int_equal(pset_size(head->modes), 3);

	head_release_mode(mode_releasing);

	assert_int_equal(pset_size(head->modes), 2);

	assert_false(pset_contains(head->modes, mode_releasing));

	assert_mode_equal(head->current.mode, mode_current);
	assert_ptr_equal(head->current.mode, mode_current);
	assert_true(pset_contains(head->modes, mode_current));

	assert_mode_equal(head->desired.mode, mode_desired);
	assert_ptr_equal(head->desired.mode, mode_desired);
	assert_true(pset_contains(head->modes, mode_desired));

	head_free(head);
}

static void head_release_mode__cur_des(void **state) {
	struct Head *head = head_init();

	struct Mode *mode_releasing = mode_h(head);
	pset_add(head->modes, mode_releasing);

	head->current.mode = mode_releasing;
	head->desired.mode = mode_releasing;

	head_release_mode(mode_releasing);

	assert_int_equal(pset_size(head->modes), 0);

	assert_false(pset_contains(head->modes, mode_releasing));

	assert_nul(head->current.mode);
	assert_nul(head->desired.mode);

	head_free(head);
}

static void head_release_mode__orphan(void **state) {
	struct Head *head = head_init();

	struct Mode *mode_releasing = mode_h(head);

	head_release_mode(mode_releasing);

	head_free(head);
}

static void head_set_mode_preferred__first(void **state) {
	struct Head *head = head_init();
	const struct Mode *mode_existing = mode_h_whr(head, 3840, 2160, 60000);
	struct Mode *mode_pref = mode_h_whr(head, 2560, 1440, 30000);

	pset_add(head->modes, mode_existing);
	pset_add(head->modes, mode_pref);

	head_set_mode_preferred(mode_pref);

	assert_ptr_equal(head->mode_preferred, mode_pref);

	head_free(head);
}

static void head_set_mode_preferred__current(void **state) {
	struct Head *head = head_init();
	head->mode_preferred = mode_h_whr(head, 2560, 1440, 30000);
	pset_add(head->modes, head->mode_preferred);

	head_set_mode_preferred(head->mode_preferred);

	assert_ptr_equal(head->mode_preferred, head->mode_preferred);

	head_free(head);
}

static void head_set_mode_preferred__subsequent(void **state) {
	struct Head *head = head_n("NAM");
	struct Mode *mode_existing = mode_h_whr(head, 3840, 2160, 60000);
	const struct Mode *mode_subsequent = mode_h_whr(head, 2560, 1440, 30000);

	head->mode_preferred = mode_existing;

	pset_add(head->modes, mode_existing);
	pset_add(head->modes, mode_subsequent);

	head_set_mode_preferred(mode_subsequent);

	assert_log(INFO, "\nNAM: multiple preferred modes advertised: using initial 3840x2160@60Hz (60,000mHz) (preferred), ignoring 2560x1440@30Hz (30,000mHz)\n");

	assert_ptr_equal(head->mode_preferred, mode_existing);

	head_free(head);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(head_get_fixed_scale__rounding_nearest),
		TEST_BA(head_get_fixed_scale__rounding_up),
		TEST_BA(head_get_fixed_scale__rounding_down),

		TEST_BA(head_auto_scale__default),
		TEST_BA(head_auto_scale__mode),
		TEST_BA(head_auto_scale__range),

		TEST_BA(head_find_mode__no_modes),
		TEST_BA(head_find_mode__all_failed),
		TEST_BA(head_find_mode__user_available),
		TEST_BA(head_find_mode__user_failed),
		TEST_BA(head_find_mode__preferred),
		TEST_BA(head_find_mode__mode_max_refresh),
		TEST_BA(head_find_mode__max),
		TEST_BA(head_find_mode__none),

		TEST_BA(head_max_mode__max),

		TEST_BA(head_matches_name_desc_regex__name),
		TEST_BA(head_matches_name_desc_regex__desc),
		TEST_BA(head_matches_name_desc_regex__bad),

		TEST_BA(head_apply_toggles__none),
		TEST_BA(head_apply_toggles__disabled__enable),
		TEST_BA(head_apply_toggles__disabled__disable),

		TEST_BA(head_set_description__nulls),
		TEST_BA(head_set_description__no_nulls),
		TEST_BA(head_set_description__empty),
		TEST_BA(head_set_description__null_input),

		TEST_BA(heads_reapply__),

		TEST_BA(head_release_mode__nulls),
		TEST_BA(head_release_mode__other),
		TEST_BA(head_release_mode__cur_des),
		TEST_BA(head_release_mode__orphan),

		TEST_BA(head_set_mode_preferred__first),
		TEST_BA(head_set_mode_preferred__current),
		TEST_BA(head_set_mode_preferred__subsequent)
	};

	return RUN(tests);
}

