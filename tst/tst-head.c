#include "tst.h"

#include "assert-log.h"
#include "assert-mode.h"
#include "assert-ppmap.h"
#include "assert-wl.h"
#include "asserts.h"
#include "data.h"
#include "expects.h"
#include "util-col.h"
#include "util-file.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cfg/cfg.h"
#include "enum.h"
#include "mode.h"
#include "ppmap.h"
#include "pset.h"
#include "spmap.h"
#include "sset.h"

#include "head.h"

static int before_each(void **state) {
	g_cfg = cfg_default();
	return 0;
}

static int after_each(void **state) {
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

	assert_logs_empty();
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

	assert_logs_empty();
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

	assert_logs_empty();
}

static void head_auto_scale__default(void **state) {
	struct Head *head = head_init();

	// no head
	assert_wl_fixed_t_equal_double(head_auto_scale(NULL, 1.0f, -1.0f), 1);

	// no desired mode
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, -1.0f), 1);

	head_free(head);

	assert_logs_empty();
}

static void head_auto_scale__mode(void **state) {
	struct Head *head = head_init();
	head->width_mm = 200;
	head->height_mm = 100;

	struct Mode *mode = mode_init();
	ppmap_put(head->modes, MD, mode);
	head->desired.zwlr_mode = MD;

	expect_ptr_count(__wrap_mode_dpi, mode, mode, 4);
	expect_int_value_count(__wrap_mode_dpi, width_mm, 200, 4);
	expect_int_value_count(__wrap_mode_dpi, height_mm, 100, 4);

	// dpi 0 defaults to 96
	will_return_int(__wrap_mode_dpi, 0);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, -1.0f), 1);

	// even 144
	will_return_int(__wrap_mode_dpi, 144);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, -1.0f), 144.0 / 96);

	// rounded down to 156
	will_return_int(__wrap_mode_dpi, 161);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, -1.0f), 156.0 / 96);

	// rounded up to 168
	will_return_int(__wrap_mode_dpi, 162);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, -1.0f), 168.0 / 96);

	head_free(head);

	assert_logs_empty();
}

static void head_auto_scale__range(void **state) {
	struct Head *head = head_init();
	head->width_mm = 400;
	head->height_mm = 300;

	struct Mode *mode = mode_init();
	ppmap_put(head->modes, MD, mode);
	head->desired.zwlr_mode = MD;

	expect_ptr_count(__wrap_mode_dpi, mode, mode, 7);
	expect_int_value_count(__wrap_mode_dpi, width_mm, 400, 7);
	expect_int_value_count(__wrap_mode_dpi, height_mm, 300, 7);

	// scale under 1.0 is clamped to 1.0 with default settings
	will_return_int(__wrap_mode_dpi, 72);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, -1.0f), 1);

	// clamping to some other minimum value works too
	will_return_int(__wrap_mode_dpi, 12);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 0.125f, -1.0f), 0.125f);

	// the minimum value is always positive (quantized to 1/8)
	will_return_int(__wrap_mode_dpi, 1);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, -1.0f, -1.0f), 0.125f);

	// clamping to maximum value works
	will_return_int(__wrap_mode_dpi, 384);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, 2.5f), 2.5f);

	// maximum values under 1.0 are ignored
	will_return_int(__wrap_mode_dpi, 384);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, 0.9f), 4.0f);

	// the configured maximum is respected even with quantization
	will_return_int(__wrap_mode_dpi, 384);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 0.63f, 1.49f), 1.375f);

	// the configured minimum is respected even with quantization
	will_return_int(__wrap_mode_dpi, 12);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 0.63f, -1.0f), 0.75f);

	head_free(head);

	assert_logs_empty();
}

static void head_find_mode__no_modes(void **state) {
	struct Head *head = head_n("head0");

	expect_int_value(__wrap_callback, t, ERROR);
	expect_str(__wrap_callback, msg1, "head0");
	expect_str(__wrap_callback, msg2, "\n  No mode, disabling");

	assert_nul(head_find_mode(head));

	assert_log(ERROR, "\nNo mode for head0, disabling.\n");

	head_free(head);

	assert_logs_empty();
}

static void head_find_mode__all_failed(void **state) {
	struct Head *head = head_n("head0");
	const struct Mode *mode = mode_init();

	// all modes failed
	ppmap_put(head->modes_failed, M0, mode);

	expect_int_value(__wrap_callback, t, ERROR);
	expect_str(__wrap_callback, msg1, "head0");
	expect_str(__wrap_callback, msg2, "\n  No mode, disabling");

	assert_nul(head_find_mode(head));

	assert_log(ERROR, "\nNo mode for head0, disabling.\n");

	head_free(head);

	assert_logs_empty();
}

static void head_find_mode__user_available(void **state) {
	struct Head *head = head_init();
	const struct Mode *mode = mode_whr(1, 2, 3);
	ppmap_put(head->modes, M0, mode);

	// user preferred head
	struct Mode *mode_target = mode_init();
	spmap_put(g_cfg->modes, "!.*EAD", mode_target);
	head->name = strdup("HEAD");

	// mode matched to user
	const struct Mode *expected = mode_whr(4, 5, 6);
	ppmap_put(head->modes, M1, expected);

	expect_ptr(__wrap_mode_best_satisfying, mode_target, mode_target);
	expect_ptr(__wrap_mode_best_satisfying, modes, head->modes);
	will_return_ptr_type(__wrap_mode_best_satisfying, expected, struct Mode*);

	const struct zwlr_output_mode_v1 *actual = head_find_mode(head);

	assert_mode_equal(ppmap_get(head->modes, actual), expected);
	assert_ptr_equal(actual, M1);

	head_free(head);

	assert_logs_empty();
}

static void head_find_mode__user_failed(void **state) {
	struct Head *head = head_init();
	const struct Mode *expected = mode_init();
	ppmap_put(head->modes, M0, expected);

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
	const struct zwlr_output_mode_v1 *actual = head_find_mode(head);

	assert_mode_equal(ppmap_get(head->modes, actual), expected);
	assert_ptr_equal(actual, M0);

	// one and only notices: falling back to preferred then max
	assert_log(WARNING, "\nHEAD: No available mode for user MODE -1x-1, falling back to preferred\n");
	assert_log(INFO, "\nHEAD: No preferred mode, falling back to maximum available\n");

	// same test again
	expect_ptr(__wrap_mode_best_satisfying, mode_target, mode_target);
	expect_ptr(__wrap_mode_best_satisfying, modes, head->modes);
	will_return_ptr_type(__wrap_mode_best_satisfying, NULL, struct Mode*);

	// marked failures avoided
	actual = head_find_mode(head);

	assert_mode_equal(ppmap_get(head->modes, actual), expected);
	assert_ptr_equal(actual, M0);

	// no notices this time

	head_free(head);

	assert_logs_empty();
}

static void head_find_mode__preferred(void **state) {
	struct Head *head = head_n("name");
	const struct Mode *expected = mode_init();

	ppmap_put(head->modes, M0, expected);
	head->zwlr_mode_pref = M0;

	const struct zwlr_output_mode_v1 *actual = head_find_mode(head);

	assert_mode_equal(ppmap_get(head->modes, actual), expected);
	assert_ptr_equal(actual, M0);

	head_free(head);

	assert_logs_empty();
}

static void head_find_mode__mode_max_refresh(void **state) {
	struct Head *head = head_n("name");
	struct Mode *expected = mode_init();

	sset_add(g_cfg->max_preferred_refresh, "!nam.*");

	ppmap_put(head->modes, M0, expected);
	head->zwlr_mode_pref = M0;

	expect_ptr(__wrap_mode_max_refresh, mode_target, expected);
	expect_ptr(__wrap_mode_max_refresh, modes, head->modes);
	will_return_ptr_type(__wrap_mode_max_refresh, expected, struct Mode*);

	const struct zwlr_output_mode_v1 *actual = head_find_mode(head);

	assert_mode_equal(ppmap_get(head->modes, actual), expected);
	assert_ptr_equal(actual, M0);

	head_free(head);

	assert_logs_empty();
}

static void head_find_mode__max(void **state) {
	struct Head *head = head_n("name");

	const struct Mode *expected = mode_init();
	ppmap_put(head->modes, M0, expected);

	expect_int_value(__wrap_callback, t, WARNING);
	expect_str(__wrap_callback, msg1, "name\n  No preferred mode, falling back to maximum available");
	expect_str(__wrap_callback, msg2, NULL);

	// one and only notice
	const struct zwlr_output_mode_v1 *actual = head_find_mode(head);

	assert_mode_equal(ppmap_get(head->modes, actual), expected);
	assert_ptr_equal(actual, M0);

	assert_log(INFO, "\nname: No preferred mode, falling back to maximum available\n");

	// no notice
	actual = head_find_mode(head);

	assert_mode_equal(ppmap_get(head->modes, actual), expected);
	assert_ptr_equal(actual, M0);

	head_free(head);

	assert_logs_empty();
}

static void head_find_mode__none(void **state) {
	struct Head *head = head_n("head0");

	expect_int_value(__wrap_callback, t, ERROR);
	expect_str(__wrap_callback, msg1, "head0");
	expect_str(__wrap_callback, msg2, "\n  No mode, disabling");

	assert_nul(head_find_mode(head));

	assert_log(ERROR, "\nNo mode for head0, disabling.\n");

	head_free(head);

	assert_logs_empty();
}

static void head_max_mode__max(void **state) {
	struct Head *head = head_init();

	struct Mode *mode_expected = mode_whr(2000, 2000, 2000);

	ppmap_put_many(head->modes,
			M0, mode_whr(1000, 1000, 1000),
			M1, mode_whr(500, 500, 1000),
			M2, mode_whr(1000, 1000, 500),
			MP, mode_whr(2000, 2000, 1000),
			M3, mode_expected,
			M4, mode_whr(1000, 1000, 1000),
			NULL);

	const struct zwlr_output_mode_v1 *actual = head_max_mode(head);

	assert_mode_equal(ppmap_get(head->modes, actual), mode_expected);
	assert_ptr_equal(actual, M3);

	head_free(head);

	assert_logs_empty();
}

static void head_matches_name_desc_regex__name(void **state) {
	struct Head *head = head_n("name");

	assert_true(head_matches_name_desc_regex(head, "!nam"));

	head_free(head);

	assert_logs_empty();
}

static void head_matches_name_desc_regex__desc(void **state) {
	struct Head *head = head_n("name");
	head->description = strdup("desc");

	assert_true(head_matches_name_desc_regex(head, "!esc"));

	head_free(head);

	assert_logs_empty();
}

static void head_matches_name_desc_regex__bad(void **state) {
	struct Head *head = head_n("name");

	assert_false(head_matches_name_desc_regex(head, "!(badregex"));

	assert_log(DEBUG, "Could not compile Head NAME_DESC regex '(badregex': Unmatched ( or \\(\n");

	head_free(head);

	assert_logs_empty();
}

static void head_matches_name_desc_regex__no_regex(void **state) {
	struct Head *head = head_n("name");

	assert_false(head_matches_name_desc_regex(head, "name"));

	head_free(head);

	assert_logs_empty();
}

static void head_apply_toggles__none(void **state) {
	struct Head *head = head_n("head0");
	struct Cfg *cfg = cfg_init();

	head_apply_toggles(head, cfg);

	assert_true(head->overrided_enabled == NoOverride);

	cfg_free(cfg);

	head_free(head);

	assert_logs_empty();
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

	assert_logs_empty();
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

	assert_logs_empty();
}

static void head_set_description__nulls(void **state) {
	struct Head *head = head_d("orig");

	head_set_description(head, "(null) (null) (null) foo (null) bar baz");

	assert_str_equal(head->description, "foo (null) bar baz");

	head_free(head);

	assert_logs_empty();
}

static void head_set_description__no_nulls(void **state) {
	struct Head *head = head_d("orig");

	head_set_description(head, "foo");

	assert_str_equal(head->description, "foo");

	head_free(head);

	assert_logs_empty();
}

static void head_set_description__empty(void **state) {
	struct Head *head = head_d("orig");

	head_set_description(head, "");

	assert_str_equal(head->description, "");

	head_free(head);

	assert_logs_empty();
}

static void head_set_description__null_input(void **state) {
	struct Head *head = head_d("orig");

	head_set_description(head, NULL);

	assert_nul(head->description);

	head_free(head);

	assert_logs_empty();
}

static void heads_reapply__(void **state) {
	const struct PPmap *heads = head_ppmap_init();

	struct Head *head_disabled = head_n("DP-7");
	head_disabled->current.enabled = false;

	const struct PPmap *modes_once_failed = mode_ppmap_init();
	ppmap_put_many(modes_once_failed,
			M0, mode_whr(3440, 1440, 59999),
			M1, mode_whr(3840, 2160, 30000),
			M2, mode_whr(3840, 2160, 29970),
			NULL);

	head_disabled->zwlr_mode_pref = M0;

	ppmap_free(head_disabled->modes_failed);
	head_disabled->modes_failed = ppmap_clone(modes_once_failed);

	ppmap_put(heads, H0, head_disabled);


	struct Head *head_enabled = head_n("eDP-1");
	head_enabled->current.enabled = true;

	ppmap_put(head_enabled->modes, MP, mode_whr(2256, 1504, 59999));
	head_enabled->current.zwlr_mode = MP;
	head_enabled->zwlr_mode_pref = MP;

	ppmap_put(heads, H1, head_enabled);


	heads_reapply(heads);


	assert_ppmap_equal(head_disabled->modes, modes_once_failed);
	assert_int_equal(ppmap_size(head_disabled->modes_failed), 0);

	char *expected_log = read_file("tst/head/reapply.log");
	assert_log(INFO, expected_log);
	free(expected_log);

	ppmap_free_vals(heads);
	ppmap_free(modes_once_failed);

	assert_logs_empty();
}

static void head_release_mode__other(void **state) {
	struct Head *head = head_init();

	ppmap_put_many(head->modes,
			MR, mode_whr(10, 20, 0),
			MC, mode_whr(30, 40, 0),
			MD, mode_whr(50, 60, 0),
			NULL
			);

	head->current.zwlr_mode = MC;
	head->desired.zwlr_mode = MD;

	assert_int_equal(ppmap_size(head->modes), 3);

	head_release_mode(head, MR);

	assert_int_equal(ppmap_size(head->modes), 2);

	assert_false(ppmap_contains_key(head->modes, M0));

	assert_ptr_equal(head->current.zwlr_mode, MC);
	assert_true(ppmap_contains_key(head->modes, MC));

	assert_ptr_equal(head->desired.zwlr_mode, MD);
	assert_true(ppmap_contains_key(head->modes, MD));

	head_free(head);

	assert_logs_empty();
}

static void head_release_mode__cur_des(void **state) {
	struct Head *head = head_init();

	ppmap_put(head->modes, MR, mode_init());
	head->current.zwlr_mode = MR;
	head->desired.zwlr_mode = MR;

	head_release_mode(head, MR);

	assert_int_equal(ppmap_size(head->modes), 0);

	assert_false(ppmap_contains_key(head->modes, MR));

	assert_nul(head->current.zwlr_mode);
	assert_nul(head->desired.zwlr_mode);

	head_free(head);

	assert_logs_empty();
}

static void head_set_mode_pref__first(void **state) {
	struct Head *head = head_init();
	ppmap_put(head->modes, M0, mode_whr(3840, 2160, 60000));
	ppmap_put(head->modes, M1, mode_whr(2560, 1440, 30000));

	head_set_mode_pref(head, M1);

	assert_ptr_equal(head->zwlr_mode_pref, M1);

	head_free(head);

	assert_logs_empty();
}

static void head_set_mode_pref__current(void **state) {
	struct Head *head = head_init();
	ppmap_put(head->modes, M0, mode_whr(2560, 1440, 30000));
	head->zwlr_mode_pref = M0;

	head_set_mode_pref(head, M0);

	assert_ptr_equal(head->zwlr_mode_pref, M0);

	head_free(head);

	assert_logs_empty();
}

static void head_set_mode_pref__subsequent(void **state) {
	struct Head *head = head_n("NAM");
	ppmap_put(head->modes, M0, mode_whr(3840, 2160, 60000));
	ppmap_put(head->modes, M1, mode_whr(2560, 1440, 30000));

	head->zwlr_mode_pref = M0;

	head_set_mode_pref(head, M1);

	assert_log(INFO, "\nNAM: multiple preferred modes advertised: using initial 3840x2160@60Hz (60,000mHz) (preferred), ignoring 2560x1440@30Hz (30,000mHz)\n");

	assert_ptr_equal(head->zwlr_mode_pref, M0);

	head_free(head);

	assert_logs_empty();
}

static void head_scale__default(void **state) {
	struct Head *head = head_init();
	head->width_mm = 1000;
	head->height_mm = 500;

	ppmap_put(head->modes, M0, mode_whr(2000, 1000, 0));

	expect_ptr(__wrap_mode_dpi, mode, ppmap_get(head->modes, M0));
	expect_int_value(__wrap_mode_dpi, width_mm, 1000);
	expect_int_value(__wrap_mode_dpi, height_mm, 500);
	will_return_int(__wrap_mode_dpi, 192);

	assert_float_equal(head_scale(head, M0), 2, 0);

	head_free(head);

	assert_logs_empty();
}

static void head_scale__cfg(void **state) {
	struct Head *head = head_init();
	head->width_mm = 1000;
	head->height_mm = 500;

	ppmap_put(head->modes, M0, mode_whr(2000, 1000, 0));

	g_cfg->auto_scale_dpi = 48;

	expect_ptr(__wrap_mode_dpi, mode, ppmap_get(head->modes, M0));
	expect_int_value(__wrap_mode_dpi, width_mm, 1000);
	expect_int_value(__wrap_mode_dpi, height_mm, 500);
	will_return_int(__wrap_mode_dpi, 192);

	assert_float_equal(head_scale(head, M0), 4, 0);

	head_free(head);

	assert_logs_empty();
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
		TEST_BA(head_matches_name_desc_regex__no_regex),

		TEST_BA(head_apply_toggles__none),
		TEST_BA(head_apply_toggles__disabled__enable),
		TEST_BA(head_apply_toggles__disabled__disable),

		TEST_BA(head_set_description__nulls),
		TEST_BA(head_set_description__no_nulls),
		TEST_BA(head_set_description__empty),
		TEST_BA(head_set_description__null_input),

		TEST_BA(heads_reapply__),

		TEST_BA(head_release_mode__other),
		TEST_BA(head_release_mode__cur_des),

		TEST_BA(head_set_mode_pref__first),
		TEST_BA(head_set_mode_pref__current),
		TEST_BA(head_set_mode_pref__subsequent),

		TEST_BA(head_scale__default),
		TEST_BA(head_scale__cfg),
	};

	return RUN(tests);
}

