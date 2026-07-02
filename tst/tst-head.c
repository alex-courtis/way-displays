#include "tst.h"

#include "assert-log.h"
#include "assert-wl.h"
#include "asserts.h"
#include "expects.h"
#include "util-file.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "cfg.h"
#include "cfg/disabled.h"
#include "cfg/user-mode.h"
#include "fn.h"
#include "log.h"
#include "mode.h"
#include "pset.h"
#include "slist.h"
#include "smap.h"
#include "sset.h"

#include "head.h"

// cppcheck-suppress staticFunction
double __wrap_mode_dpi(const struct WlrMode* const wlr_mode) {
	check_expected_ptr(wlr_mode);
	return mock_type(double);
}

// cppcheck-suppress staticFunction
const struct WlrMode *__wrap_mode_user_mode(const struct PSet* const wlr_modes, struct SList *wlr_modes_failed, const struct UserMode *user_mode) {
	check_expected_ptr(wlr_modes);
	check_expected_ptr(wlr_modes_failed);
	check_expected_ptr(user_mode);
	return mock_ptr_type_checked(struct WlrMode*);
}

// cppcheck-suppress staticFunction
const struct WlrMode *__wrap_mode_max_preferred(const struct PSet* wlr_modes, struct SList *wlr_modes_failed) {
	check_expected_ptr(wlr_modes);
	check_expected_ptr(wlr_modes_failed);
	return mock_ptr_type_checked(struct WlrMode*);
}

static int before_each(void **state) {
	// assert_logs_empty_before();

	g_cfg = cfg_default();
	return 0;
}

static int after_each(void **state) {
	cfg_destroy();
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

	assert_logs_empty();

	head_free(head);
}

static void head_auto_scale__mode(void **state) {
	struct Head *head = head_init();

	struct WlrMode *wlr_mode = wlr_mode_init_head(head);
	head->desired.wlr_mode = wlr_mode;
	pset_add(head->wlr_modes, wlr_mode);

	// dpi 0 defaults to 96
	expect_ptr(__wrap_mode_dpi, wlr_mode, wlr_mode);
	will_return_int(__wrap_mode_dpi, 0);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, -1.0f), 1);

	// even 144
	expect_ptr(__wrap_mode_dpi, wlr_mode, wlr_mode);
	will_return_int(__wrap_mode_dpi, 144);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, -1.0f), 144.0 / 96);

	// rounded down to 156
	expect_ptr(__wrap_mode_dpi, wlr_mode, wlr_mode);
	will_return_int(__wrap_mode_dpi, 161);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, -1.0f), 156.0 / 96);

	// rounded up to 168
	expect_ptr(__wrap_mode_dpi, wlr_mode, wlr_mode);
	will_return_int(__wrap_mode_dpi, 162);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, -1.0f), 168.0 / 96);

	assert_logs_empty();

	head_free(head);
}

static void head_auto_scale__range(void **state) {
	struct Head *head = head_init();

	struct WlrMode *wlr_mode = wlr_mode_init_head(head);
	head->desired.wlr_mode = wlr_mode;
	pset_add(head->wlr_modes, wlr_mode);

	// scale under 1.0 is clamped to 1.0 with default settings
	expect_ptr(__wrap_mode_dpi, wlr_mode, wlr_mode);
	will_return_int(__wrap_mode_dpi, 72);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, -1.0f), 1);

	// clamping to some other minimum value works too
	expect_ptr(__wrap_mode_dpi, wlr_mode, wlr_mode);
	will_return_int(__wrap_mode_dpi, 12);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 0.125f, -1.0f), 0.125f);

	// the minimum value is always positive (quantized to 1/8)
	expect_ptr(__wrap_mode_dpi, wlr_mode, wlr_mode);
	will_return_int(__wrap_mode_dpi, 1);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, -1.0f, -1.0f), 0.125f);

	// clamping to maximum value works
	expect_ptr(__wrap_mode_dpi, wlr_mode, wlr_mode);
	will_return_int(__wrap_mode_dpi, 384);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, 2.5f), 2.5f);

	// maximum values under 1.0 are ignored
	expect_ptr(__wrap_mode_dpi, wlr_mode, wlr_mode);
	will_return_int(__wrap_mode_dpi, 384);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 1.0f, 0.9f), 4.0f);

	// the configured maximum is respected even with quantization
	expect_ptr(__wrap_mode_dpi, wlr_mode, wlr_mode);
	will_return_int(__wrap_mode_dpi, 384);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 0.63f, 1.49f), 1.375f);

	// the configured minimum is respected even with quantization
	expect_ptr(__wrap_mode_dpi, wlr_mode, wlr_mode);
	will_return_int(__wrap_mode_dpi, 12);
	assert_wl_fixed_t_equal_double(head_auto_scale(head, 0.63f, -1.0f), 0.75f);

	assert_logs_empty();

	head_free(head);
}

static void head_set_scaled_dimensions__default(void **state) {
	struct Head *head = head_init();
	head->scaled.width = 1;
	head->scaled.height = 1;

	// no head
	head_set_scaled_dimensions(NULL);

	// no mode
	head_set_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 1);
	assert_int_equal(head->scaled.height, 1);

	// no scale
	const struct WlrMode *wlr_mode = wlr_mode_init(head, NULL, 200, 100, 0, false);
	head->desired.wlr_mode = wlr_mode;
	pset_add(head->wlr_modes, wlr_mode);

	head_set_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 1);
	assert_int_equal(head->scaled.height, 1);

	assert_logs_empty();

	head_free(head);
}

static void head_set_scaled_dimensions__transform(void **state) {
	struct Head *head = head_init();

	const struct WlrMode *wlr_mode = wlr_mode_init(head, NULL, 200, 100, 0, false);
	head->desired.wlr_mode = wlr_mode;
	pset_add(head->wlr_modes, wlr_mode);

	// double, not rotated
	head->desired.scale = wl_fixed_from_double(0.5);
	head->desired.transform = WL_OUTPUT_TRANSFORM_180;

	head_set_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 400);
	assert_int_equal(head->scaled.height, 200);

	// one third, rotated
	head->desired.scale = wl_fixed_from_double(3);
	head->desired.transform = WL_OUTPUT_TRANSFORM_90;

	head_set_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 33);
	assert_int_equal(head->scaled.height, 66); // wayland truncates when calculating size

	assert_logs_empty();

	head_free(head);
}

static void head_set_scaled_dimensions__dimensions(void **state) {
	struct Head *head = head_init();

	const struct WlrMode *wlr_mode = wlr_mode_init(head, NULL, 3840, 2160, 0, false);
	head->desired.wlr_mode = wlr_mode;
	pset_add(head->wlr_modes, wlr_mode);

	head->desired.scale = head_get_fixed_scale(1.0);
	head_set_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 3840);
	assert_int_equal(head->scaled.height, 2160);
	assert_logs_empty();

	head->desired.scale = head_get_fixed_scale(2.0);
	head_set_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 1920);
	assert_int_equal(head->scaled.height, 1080);
	assert_logs_empty();

	head->desired.scale = head_get_fixed_scale(1.7);
	// actual scale will be 1.75
	head_set_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 2194);
	assert_int_equal(head->scaled.height, 1234);
	assert_logs_empty();

	head->desired.scale = head_get_fixed_scale(1.9);
	// actual scale will be 1.875
	head_set_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 2048);
	assert_int_equal(head->scaled.height, 1152);
	assert_logs_empty();

	head->name = strdup("name");

	head->desired.scale = head_get_fixed_scale(2.01);
	// actual scale will be 2.0
	head_set_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 1920);
	assert_int_equal(head->scaled.height, 1080);
	assert_logs_empty();

	head_free(head);
}

static void head_find_mode__all_failed(void **state) {
	struct Head *head = head_init_name("head0");
	struct WlrMode *wlr_mode = wlr_mode_init_head(head);
	pset_add(head->wlr_modes, wlr_mode);

	// all modes failed
	slist_append(&head->wlr_modes_failed, wlr_mode);

	expect_int_value(__wrap_call_back, t, ERROR);
	expect_str(__wrap_call_back, msg1, "head0");
	expect_str(__wrap_call_back, msg2, "\n  No mode, disabling");

	assert_nul(head_find_wlr_mode(head));

	assert_log(ERROR, "\nNo mode for head0, disabling.\n");

	head_free(head);
}

static void head_find_mode__user_available(void **state) {
	struct Head *head = head_init();
	const struct WlrMode *wlr_mode = wlr_mode_init_head(head);
	pset_add(head->wlr_modes, wlr_mode);

	// user preferred head
	struct UserMode *user_mode = user_mode_init_default();
	smap_put(g_cfg->user_modes, "!.*EAD", user_mode);
	head->name = strdup("HEAD");

	// mode matched to user
	const struct WlrMode *expected = wlr_mode_init_head(head);
	pset_add(head->wlr_modes, expected);

	expect_ptr(__wrap_mode_user_mode, wlr_modes, head->wlr_modes);
	expect_ptr(__wrap_mode_user_mode, wlr_modes_failed, head->wlr_modes_failed);
	expect_ptr(__wrap_mode_user_mode, user_mode, user_mode);
	will_return_ptr_type(__wrap_mode_user_mode, expected, struct WlrMode*);

	assert_ptr_equal(head_find_wlr_mode(head), expected);

	head_free(head);

	assert_logs_empty();
}

static void head_find_mode__user_failed(void **state) {
	struct Head *head = head_init();
	struct WlrMode *wlr_mode = wlr_mode_init_head(head);
	pset_add(head->wlr_modes, wlr_mode);

	// user preferred head
	struct UserMode *user_mode = user_mode_init_default();
	smap_put(g_cfg->user_modes, "!HEA.*", user_mode);
	head->name = strdup("HEAD");

	// mode not matched to user
	expect_ptr(__wrap_mode_user_mode, wlr_modes, head->wlr_modes);
	expect_ptr(__wrap_mode_user_mode, wlr_modes_failed, head->wlr_modes_failed);
	expect_ptr(__wrap_mode_user_mode, user_mode, user_mode);
	will_return_ptr_type(__wrap_mode_user_mode, NULL, struct WlrMode*);

	expect_int_value(__wrap_call_back, t, WARNING);
	expect_str(__wrap_call_back, msg1, "HEAD\n  No available mode for user MODE -1x-1, falling back to preferred");
	expect_str(__wrap_call_back, msg2, NULL);

	expect_int_value(__wrap_call_back, t, WARNING);
	expect_str(__wrap_call_back, msg1, "HEAD\n  No preferred mode, falling back to maximum available");
	expect_str(__wrap_call_back, msg2, NULL);

	// user failed, fall back to max
	assert_ptr_equal(head_find_wlr_mode(head), wlr_mode);

	// one and only notices: falling back to preferred then max
	assert_log(WARNING, "\nHEAD: No available mode for user MODE -1x-1, falling back to preferred\n");
	assert_log(INFO, "\nHEAD: No preferred mode, falling back to maximum available\n");
	assert_logs_empty();

	// same test again
	expect_ptr(__wrap_mode_user_mode, wlr_modes, head->wlr_modes);
	expect_ptr(__wrap_mode_user_mode, wlr_modes_failed, head->wlr_modes_failed);
	expect_ptr(__wrap_mode_user_mode, user_mode, user_mode);
	will_return_ptr_type(__wrap_mode_user_mode, NULL, struct WlrMode*);

	// marked failures avoided
	assert_ptr_equal(head_find_wlr_mode(head), wlr_mode);

	// no notices this time
	assert_logs_empty();

	head_free(head);
}

static void head_find_mode__preferred(void **state) {
	struct Head *head = head_init_name("name");
	struct WlrMode *wlr_mode = wlr_mode_init_empty();
	wlr_mode->preferred = true;

	pset_add(head->wlr_modes, wlr_mode);

	assert_ptr_equal(head_find_wlr_mode(head), wlr_mode);

	head_free(head);

	assert_logs_empty();
}

static void head_find_mode__max_preferred_refresh(void **state) {
	struct Head *head = head_init_name("name");
	struct WlrMode *wlr_mode = wlr_mode_init_head(head);

	sset_add(g_cfg->max_preferred_refresh, "!nam.*");

	pset_add(head->wlr_modes, wlr_mode);

	expect_ptr(__wrap_mode_max_preferred, wlr_modes, head->wlr_modes);
	expect_ptr(__wrap_mode_max_preferred, wlr_modes_failed, head->wlr_modes_failed);
	will_return_ptr_type(__wrap_mode_max_preferred, &wlr_mode, struct WlrMode*);

	assert_ptr_equal(head_find_wlr_mode(head), &wlr_mode);

	head_free(head);

	assert_logs_empty();
}

static void head_find_mode__max(void **state) {
	struct Head *head = head_init_name("name");

	struct WlrMode *wlr_mode = wlr_mode_init_head(head);
	pset_add(head->wlr_modes, wlr_mode);

	expect_int_value(__wrap_call_back, t, WARNING);
	expect_str(__wrap_call_back, msg1, "name\n  No preferred mode, falling back to maximum available");
	expect_str(__wrap_call_back, msg2, NULL);

	// one and only notice
	assert_ptr_equal(head_find_wlr_mode(head), wlr_mode);
	assert_log(INFO, "\nname: No preferred mode, falling back to maximum available\n");
	assert_logs_empty();

	// no notice
	assert_ptr_equal(head_find_wlr_mode(head), wlr_mode);

	head_free(head);
}

static void head_find_mode__none(void **state) {
	struct Head *head = head_init_name("head0");

	struct WlrMode *wlr_mode_failed = wlr_mode_init_head(head);

	// force to pass the first check and skip preferred messages
	slist_append(&head->wlr_modes_failed, wlr_mode_failed);
	head->warned_no_preferred = true;

	expect_int_value(__wrap_call_back, t, ERROR);
	expect_str(__wrap_call_back, msg1, "head0");
	expect_str(__wrap_call_back, msg2, "\n  No mode, disabling");

	assert_nul(head_find_wlr_mode(head));

	assert_log(ERROR, "\nNo mode for head0, disabling.\n");
	assert_logs_empty();

	head_free(head);
	free(wlr_mode_failed);
}

static void head_max_mode__max(void **state) {
	struct Head *head = head_init();

	struct WlrMode *wlr_mode_failed = wlr_mode_init_head(head);
	slist_append(&head->wlr_modes_failed, wlr_mode_failed);


	pset_add(head->wlr_modes, wlr_mode_failed);
	pset_add(head->wlr_modes, wlr_mode_init(NULL, NULL, 1000, 1000, 1000, false));
	pset_add(head->wlr_modes, wlr_mode_init(NULL, NULL, 500, 500, 1000, false));
	pset_add(head->wlr_modes, wlr_mode_init(NULL, NULL, 1000, 1000, 500, false));
	pset_add(head->wlr_modes, wlr_mode_init(NULL, NULL, 2000, 2000, 1000, false));
	struct WlrMode *wlr_mode_expected = wlr_mode_init(NULL, NULL, 2000, 2000, 2000, false);
	pset_add(head->wlr_modes, wlr_mode_expected);
	pset_add(head->wlr_modes, wlr_mode_init(NULL, NULL, 1000, 1000, 1000, false));

	assert_ptr_equal(head_max_wlr_mode(head), wlr_mode_expected);

	head_free(head);
}

static void head_matches_name_desc_regex__name(void **state) {
	struct Head *head = head_init_name("name");

	assert_true(head_matches_name_desc_regex(head, "!nam"));

	assert_logs_empty();

	head_free(head);
}

static void head_matches_name_desc_regex__desc(void **state) {
	struct Head *head = head_init_name("name");
	head->description = strdup("desc");

	assert_true(head_matches_name_desc_regex(head, "!esc"));

	assert_logs_empty();

	head_free(head);
}

static void head_matches_name_desc_regex__bad(void **state) {
	struct Head *head = head_init_name("name");

	assert_false(head_matches_name_desc_regex(head, "!(badregex"));

	assert_log(DEBUG, "Could not compile Head NAME_DESC regex '(badregex': Unmatched ( or \\(\n");
	assert_logs_empty();

	head_free(head);
}

static void head_apply_toggles__none(void **state) {
	struct Head *head = head_init_name("head0");
	struct Cfg *cfg = cfg_init();

	head_apply_toggles(head, cfg);

	assert_true(head->overrided_enabled == NoOverride);

	cfg_free(cfg);

	assert_logs_empty();

	head_free(head);
}

static void head_apply_toggles__disabled__enable(void **state) {
	struct Head *head = head_init_name("head0");
	head->current.enabled = false;
	struct Cfg *cfg = cfg_init();
	pset_add(cfg->disableds, disabled_init_name_desc("head0"));

	head_apply_toggles(head, cfg);

	assert_true(head->overrided_enabled == OverrideTrue);
	assert_log(INFO, "\nApplying \"DISABLED\" override for head0\n");
	assert_logs_empty();

	head_apply_toggles(head, cfg);

	assert_true(head->overrided_enabled == NoOverride);
	assert_log(INFO, "\nResetting \"DISABLED\" override for head0\n");
	assert_logs_empty();

	cfg_free(cfg);
	head_free(head);
}

static void head_apply_toggles__disabled__disable(void **state) {
	struct Head *head = head_init_name("head0");
	head->current.enabled = true;
	struct Cfg *cfg = cfg_init();
	pset_add(cfg->disableds, disabled_init_name_desc("head0"));

	head_apply_toggles(head, cfg);

	assert_true(head->overrided_enabled == OverrideFalse);
	assert_log(INFO, "\nApplying \"DISABLED\" override for head0\n");
	assert_logs_empty();

	head_apply_toggles(head, cfg);

	assert_true(head->overrided_enabled == NoOverride);
	assert_log(INFO, "\nResetting \"DISABLED\" override for head0\n");
	assert_logs_empty();

	cfg_free(cfg);
	head_free(head);
}

static void head_set_description__nulls(void **state) {
	struct Head *head = head_init_description("orig");

	head_set_description(head, "(null) (null) (null) foo (null) bar baz");

	assert_str_equal(head->description, "foo (null) bar baz");

	head_free(head);
}

static void head_set_description__no_nulls(void **state) {
	struct Head *head = head_init_description("orig");

	head_set_description(head, "foo");

	assert_str_equal(head->description, "foo");

	head_free(head);
}

static void head_set_description__empty(void **state) {
	struct Head *head = head_init_description("orig");

	head_set_description(head, "");

	assert_str_equal(head->description, "");

	head_free(head);
}

static void head_set_description__null_input(void **state) {
	struct Head *head = head_init_description("orig");

	head_set_description(head, NULL);

	assert_nul(head->description);

	head_free(head);
}

static void heads_reapply__(void **state) {
	struct SList *heads = NULL;

	struct Head *head_disabled = head_init_name("DP-7");
	head_disabled->current.enabled = false;

	pset_add(head_disabled->wlr_modes, wlr_mode_init(NULL, NULL, 3440, 1440, 59999, true));
	pset_add(head_disabled->wlr_modes, wlr_mode_init(NULL, NULL, 3840, 2160, 30000, false));
	pset_add(head_disabled->wlr_modes, wlr_mode_init(NULL, NULL, 3840, 2160, 29970, false));

	head_disabled->wlr_modes_failed = pset_slist_shallow(head_disabled->wlr_modes);

	slist_append(&heads, head_disabled);


	struct Head *head_enabled = head_init_name("eDP-1");
	head_enabled->current.enabled = true;

	head_enabled->current.wlr_mode = wlr_mode_init(NULL, NULL, 2256, 1504, 59999, true);;
	pset_add(head_enabled->wlr_modes, head_enabled->current.wlr_mode);

	slist_append(&heads, head_enabled);


	heads_reapply(heads);


	char *expected_log = read_file("tst/head/reapply.log");
	assert_log(INFO, expected_log);
	assert_logs_empty();
	free(expected_log);


	slist_free_vals(&heads, (fn_free)head_free);
}

static void head_release_mode__nulls(void **state) {
	head_release_mode(NULL);

	struct WlrMode *wlr_mode_releasing = wlr_mode_init(NULL, NULL, 0, 0, 0, false);

	head_release_mode(wlr_mode_releasing);
}

static void head_release_mode__other(void **state) {
	struct Head *head = head_init();

	struct WlrMode *wlr_mode_releasing = wlr_mode_init(head, NULL, 0, 0, 0, false);
	pset_add(head->wlr_modes, wlr_mode_releasing);

	struct WlrMode *wlr_mode_current = wlr_mode_init(head, NULL, 0, 0, 0, false);
	pset_add(head->wlr_modes, wlr_mode_current);
	head->current.wlr_mode = wlr_mode_current;

	struct WlrMode *wlr_mode_desired = wlr_mode_init(head, NULL, 0, 0, 0, false);
	pset_add(head->wlr_modes, wlr_mode_desired);
	head->desired.wlr_mode = wlr_mode_desired;

	assert_int_equal(pset_size(head->wlr_modes), 3);

	head_release_mode(wlr_mode_releasing);

	assert_int_equal(pset_size(head->wlr_modes), 2);

	assert_false(pset_contains(head->wlr_modes, wlr_mode_releasing));

	assert_ptr_equal(head->current.wlr_mode, wlr_mode_current);
	assert_true(pset_contains(head->wlr_modes, wlr_mode_current));

	assert_ptr_equal(head->desired.wlr_mode, wlr_mode_desired);
	assert_true(pset_contains(head->wlr_modes, wlr_mode_desired));

	head_free(head);
}

static void head_release_mode__cur_des(void **state) {
	struct Head *head = head_init();

	struct WlrMode *wlr_mode_releasing = wlr_mode_init(head, NULL, 0, 0, 0, false);
	pset_add(head->wlr_modes, wlr_mode_releasing);

	head->current.wlr_mode = wlr_mode_releasing;
	head->desired.wlr_mode = wlr_mode_releasing;

	head_release_mode(wlr_mode_releasing);

	assert_int_equal(pset_size(head->wlr_modes), 0);

	assert_false(pset_contains(head->wlr_modes, wlr_mode_releasing));

	assert_nul(head->current.wlr_mode);
	assert_nul(head->desired.wlr_mode);

	head_free(head);
}

static void head_release_mode__orphan(void **state) {
	struct Head *head = head_init();

	struct WlrMode *wlr_mode_releasing = wlr_mode_init(head, NULL, 0, 0, 0, false);

	head_release_mode(wlr_mode_releasing);

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

		TEST_BA(head_set_scaled_dimensions__default),
		TEST_BA(head_set_scaled_dimensions__transform),
		TEST_BA(head_set_scaled_dimensions__dimensions),

		TEST_BA(head_find_mode__all_failed),
		TEST_BA(head_find_mode__user_available),
		TEST_BA(head_find_mode__user_failed),
		TEST_BA(head_find_mode__preferred),
		TEST_BA(head_find_mode__max_preferred_refresh),
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
	};

	return RUN(tests);
}

