#include "tst.h"
#include "asserts.h"
#include "expects.h"
#include "util.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "cfg.h"
#include "slist.h"
#include "log.h"
#include "mode.h"

#include "head.h"

double __wrap_mode_dpi(struct Mode *mode) {
	check_expected_ptr(mode);
	return mock_type(double);
}

struct Mode *__wrap_mode_user_mode(struct SList *modes, struct SList *modes_failed, struct UserMode *user_mode) {
	check_expected_ptr(modes);
	check_expected_ptr(modes_failed);
	check_expected_ptr(user_mode);
	return mock_ptr_type_checked(struct Mode*);
}

struct Mode *__wrap_mode_max_preferred(struct SList *modes, struct SList *modes_failed) {
	check_expected_ptr(modes);
	check_expected_ptr(modes_failed);
	return mock_ptr_type_checked(struct Mode*);
}


int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	logs_clear();

	g_cfg = cfg_default();
	return 0;
}

int after_each(void **state) {
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
	struct Head head = { 0 };

	// no head
	assert_wl_fixed_t_equal_double(head_auto_scale(NULL, 1.0f, -1.0f), 1);

	// no desired mode
	assert_wl_fixed_t_equal_double(head_auto_scale(&head, 1.0f, -1.0f), 1);

	assert_logs_empty();
}

static void head_auto_scale__mode(void **state) {
	struct Mode mode = { 0 };
	struct Head head = { .desired.mode = &mode };

	// dpi 0 defaults to 96
	expect_ptr(__wrap_mode_dpi, mode, &mode);
	will_return_int(__wrap_mode_dpi, 0);
	assert_wl_fixed_t_equal_double(head_auto_scale(&head, 1.0f, -1.0f), 1);

	// even 144
	expect_ptr(__wrap_mode_dpi, mode, &mode);
	will_return_int(__wrap_mode_dpi, 144);
	assert_wl_fixed_t_equal_double(head_auto_scale(&head, 1.0f, -1.0f), 144.0 / 96);

	// rounded down to 156
	expect_ptr(__wrap_mode_dpi, mode, &mode);
	will_return_int(__wrap_mode_dpi, 161);
	assert_wl_fixed_t_equal_double(head_auto_scale(&head, 1.0f, -1.0f), 156.0 / 96);

	// rounded up to 168
	expect_ptr(__wrap_mode_dpi, mode, &mode);
	will_return_int(__wrap_mode_dpi, 162);
	assert_wl_fixed_t_equal_double(head_auto_scale(&head, 1.0f, -1.0f), 168.0 / 96);

	assert_logs_empty();
}

static void head_auto_scale__range(void **state) {
	struct Mode mode = { 0 };
	struct Head head = { .desired.mode = &mode };

	// scale under 1.0 is clamped to 1.0 with default settings
	expect_ptr(__wrap_mode_dpi, mode, &mode);
	will_return_int(__wrap_mode_dpi, 72);
	assert_wl_fixed_t_equal_double(head_auto_scale(&head, 1.0f, -1.0f), 1);

	// clamping to some other minimum value works too
	expect_ptr(__wrap_mode_dpi, mode, &mode);
	will_return_int(__wrap_mode_dpi, 12);
	assert_wl_fixed_t_equal_double(head_auto_scale(&head, 0.125f, -1.0f), 0.125f);

	// the minimum value is always positive (quantized to 1/8)
	expect_ptr(__wrap_mode_dpi, mode, &mode);
	will_return_int(__wrap_mode_dpi, 1);
	assert_wl_fixed_t_equal_double(head_auto_scale(&head, -1.0f, -1.0f), 0.125f);

	// clamping to maximum value works
	expect_ptr(__wrap_mode_dpi, mode, &mode);
	will_return_int(__wrap_mode_dpi, 384);
	assert_wl_fixed_t_equal_double(head_auto_scale(&head, 1.0f, 2.5f), 2.5f);

	// maximum values under 1.0 are ignored
	expect_ptr(__wrap_mode_dpi, mode, &mode);
	will_return_int(__wrap_mode_dpi, 384);
	assert_wl_fixed_t_equal_double(head_auto_scale(&head, 1.0f, 0.9f), 4.0f);

	// the configured maximum is respected even with quantization
	expect_ptr(__wrap_mode_dpi, mode, &mode);
	will_return_int(__wrap_mode_dpi, 384);
	assert_wl_fixed_t_equal_double(head_auto_scale(&head, 0.63f, 1.49f), 1.375f);

	// the configured minimum is respected even with quantization
	expect_ptr(__wrap_mode_dpi, mode, &mode);
	will_return_int(__wrap_mode_dpi, 12);
	assert_wl_fixed_t_equal_double(head_auto_scale(&head, 0.63f, -1.0f), 0.75f);

	assert_logs_empty();
}

static void head_set_scaled_dimensions__default(void **state) {
	struct Head head = { .scaled.width = 1, .scaled.height = 1, };

	// no head
	head_set_scaled_dimensions(NULL);

	// no mode
	head_set_scaled_dimensions(&head);
	assert_int_equal(head.scaled.width, 1);
	assert_int_equal(head.scaled.height, 1);

	// no scale
	struct Mode mode = { .width = 200, .height = 100, };
	head.desired.mode = &mode;

	head_set_scaled_dimensions(&head);
	assert_int_equal(head.scaled.width, 1);
	assert_int_equal(head.scaled.height, 1);

	assert_logs_empty();
}

static void head_set_scaled_dimensions__transform(void **state) {
	struct Mode mode = { .width = 200, .height = 100, };
	struct Head head = { .desired.mode = &mode, };

	// double, not rotated
	head.desired.scale = wl_fixed_from_double(0.5);
	head.desired.transform = WL_OUTPUT_TRANSFORM_180;

	head_set_scaled_dimensions(&head);
	assert_int_equal(head.scaled.width, 400);
	assert_int_equal(head.scaled.height, 200);

	// one third, rotated
	head.desired.scale = wl_fixed_from_double(3);
	head.desired.transform = WL_OUTPUT_TRANSFORM_90;

	head_set_scaled_dimensions(&head);
	assert_int_equal(head.scaled.width, 33);
	assert_int_equal(head.scaled.height, 66); // wayland truncates when calculating size

	assert_logs_empty();
}

static void head_set_scaled_dimensions__dimensions(void **state) {
	struct Mode mode = { .width = 3840, .height = 2160, };
	struct Head head = { .desired.mode = &mode, };

	head.desired.scale = head_get_fixed_scale(1.0);
	head_set_scaled_dimensions(&head);
	assert_int_equal(head.scaled.width, 3840);
	assert_int_equal(head.scaled.height, 2160);
	assert_logs_empty();

	head.desired.scale = head_get_fixed_scale(2.0);
	head_set_scaled_dimensions(&head);
	assert_int_equal(head.scaled.width, 1920);
	assert_int_equal(head.scaled.height, 1080);
	assert_logs_empty();

	head.desired.scale = head_get_fixed_scale(1.7);
	// actual scale will be 1.75
	head_set_scaled_dimensions(&head);
	assert_int_equal(head.scaled.width, 2194);
	assert_int_equal(head.scaled.height, 1234);
	assert_logs_empty();

	head.desired.scale = head_get_fixed_scale(1.9);
	// actual scale will be 1.875
	head_set_scaled_dimensions(&head);
	assert_int_equal(head.scaled.width, 2048);
	assert_int_equal(head.scaled.height, 1152);
	assert_logs_empty();

	head.name = "name";

	head.desired.scale = head_get_fixed_scale(2.01);
	// actual scale will be 2.0
	head_set_scaled_dimensions(&head);
	assert_int_equal(head.scaled.width, 1920);
	assert_int_equal(head.scaled.height, 1080);
	assert_logs_empty();
}

static void head_find_mode__all_failed(void **state) {
	struct Head head = { .name = "head0" };
	struct Mode mode = { 0 };

	// all modes failed
	slist_append(&head.modes, &mode);
	slist_append(&head.modes_failed, &mode);

	expect_int_value(__wrap_call_back, t, ERROR);
	expect_str(__wrap_call_back, msg1, "head0");
	expect_str(__wrap_call_back, msg2, "\n  No mode, disabling");

	assert_nul(head_find_mode(&head));

	assert_log(ERROR, "\nNo mode for head0, disabling.\n");
	assert_logs_empty();

	slist_free(&head.modes);
	slist_free(&head.modes_failed);
}

static void head_find_mode__user_available(void **state) {
	struct Head head = { 0 };
	struct Mode mode = { 0 };
	slist_append(&head.modes, &mode);

	// user preferred head
	struct UserMode *user_mode = cfg_user_mode_default();
	user_mode->name_desc = strdup("!.*EAD");
	slist_append(&g_cfg->user_modes, user_mode);
	head.name = strdup("HEAD");

	// mode matched to user
	struct Mode expected = { 0 };
	expect_ptr(__wrap_mode_user_mode, modes, head.modes);
	expect_ptr(__wrap_mode_user_mode, modes_failed, head.modes_failed);
	expect_ptr(__wrap_mode_user_mode, user_mode, user_mode);
	will_return_ptr_type(__wrap_mode_user_mode, &expected, struct Mode*);

	assert_ptr_equal(head_find_mode(&head), &expected);

	slist_free(&head.modes);
	free(head.name);

	assert_logs_empty();
}

static void head_find_mode__user_failed(void **state) {
	struct Head head = { 0 };
	struct Mode mode = { 0 };
	slist_append(&head.modes, &mode);

	// user preferred head
	struct UserMode *user_mode = cfg_user_mode_default();
	user_mode->name_desc = strdup("!HEA.*");
	slist_append(&g_cfg->user_modes, user_mode);
	head.name = strdup("HEAD");

	// mode not matched to user
	expect_ptr(__wrap_mode_user_mode, modes, head.modes);
	expect_ptr(__wrap_mode_user_mode, modes_failed, head.modes_failed);
	expect_ptr(__wrap_mode_user_mode, user_mode, user_mode);
	will_return_ptr_type(__wrap_mode_user_mode, NULL, struct Mode*);

	expect_int_value(__wrap_call_back, t, WARNING);
	expect_str(__wrap_call_back, msg1, "HEAD\n  No available mode for user MODE -1x-1, falling back to preferred");
	expect_str(__wrap_call_back, msg2, NULL);

	expect_int_value(__wrap_call_back, t, WARNING);
	expect_str(__wrap_call_back, msg1, "HEAD\n  No preferred mode, falling back to maximum available");
	expect_str(__wrap_call_back, msg2, NULL);

	// user failed, fall back to max
	assert_ptr_equal(head_find_mode(&head), &mode);

	// one and only notices: falling back to preferred then max
	assert_log(WARNING, "\nHEAD: No available mode for user MODE -1x-1, falling back to preferred\n");
	assert_log(INFO, "\nHEAD: No preferred mode, falling back to maximum available\n");
	assert_logs_empty();

	// same test again
	expect_ptr(__wrap_mode_user_mode, modes, head.modes);
	expect_ptr(__wrap_mode_user_mode, modes_failed, head.modes_failed);
	expect_ptr(__wrap_mode_user_mode, user_mode, user_mode);
	will_return_ptr_type(__wrap_mode_user_mode, NULL, struct Mode*);

	// marked failures avoided
	assert_ptr_equal(head_find_mode(&head), &mode);

	// no notices this time
	assert_logs_empty();

	slist_free(&head.modes);
	free(head.name);
}

static void head_find_mode__preferred(void **state) {
	struct Head head = { .name = "name", };
	struct Mode mode = { .preferred = true, };

	slist_append(&head.modes, &mode);

	assert_ptr_equal(head_find_mode(&head), &mode);

	slist_free(&head.modes);

	assert_logs_empty();
}

static void head_find_mode__max_preferred_refresh(void **state) {
	struct Head head = { .name = "name", };
	struct Mode mode = { 0 };

	slist_append(&g_cfg->max_preferred_refresh_name_desc, strdup("!nam.*"));

	slist_append(&head.modes, &mode);

	expect_ptr(__wrap_mode_max_preferred, modes, head.modes);
	expect_ptr(__wrap_mode_max_preferred, modes_failed, head.modes_failed);
	will_return_ptr_type(__wrap_mode_max_preferred, &mode, struct Mode*);

	assert_ptr_equal(head_find_mode(&head), &mode);

	slist_free(&head.modes);

	assert_logs_empty();
}

static void head_find_mode__max(void **state) {
	struct Head head = { .name = "name", };
	struct Mode mode = { 0 };

	slist_append(&head.modes, &mode);

	expect_int_value(__wrap_call_back, t, WARNING);
	expect_str(__wrap_call_back, msg1, "name\n  No preferred mode, falling back to maximum available");
	expect_str(__wrap_call_back, msg2, NULL);

	// one and only notice
	assert_ptr_equal(head_find_mode(&head), &mode);
	assert_log(INFO, "\nname: No preferred mode, falling back to maximum available\n");
	assert_logs_empty();

	// no notice
	assert_ptr_equal(head_find_mode(&head), &mode);

	slist_free(&head.modes);
}

static void head_find_mode__none(void **state) {
	struct Head head = { .name = "head0", };
	struct Mode mode = { 0 };

	// force to pass the first check and skip preferred messages
	slist_append(&head.modes_failed, &mode);
	head.warned_no_preferred = true;

	expect_int_value(__wrap_call_back, t, ERROR);
	expect_str(__wrap_call_back, msg1, "head0");
	expect_str(__wrap_call_back, msg2, "\n  No mode, disabling");

	assert_nul(head_find_mode(&head));

	assert_log(ERROR, "\nNo mode for head0, disabling.\n");
	assert_logs_empty();

	slist_free(&head.modes_failed);
}

static void head_apply_toggles__none(void **state) {
	struct Head head = { .name = "head0", };
	struct Cfg *cfg = cfg_init();

	head_apply_toggles(&head, cfg);

	assert_true(head.overrided_enabled == NoOverride);

	cfg_free(cfg);

	assert_logs_empty();
}

static void head_apply_toggles__disabled__enable(void **state) {
	struct Head head = { .name = "head0", .current.enabled = false };
	struct Cfg *cfg = cfg_init();
	slist_append(&cfg->disabled, cfg_disabled_always("head0"));

	head_apply_toggles(&head, cfg);

	assert_true(head.overrided_enabled == OverrideTrue);
	assert_log(INFO, "\nApplying \"DISABLED\" override for head0\n");
	assert_logs_empty();

	head_apply_toggles(&head, cfg);

	assert_true(head.overrided_enabled == NoOverride);
	assert_log(INFO, "\nResetting \"DISABLED\" override for head0\n");
	assert_logs_empty();

	cfg_free(cfg);
}

static void head_apply_toggles__disabled__disable(void **state) {
	struct Head head = { .name = "head0", .current.enabled = true };
	struct Cfg *cfg = cfg_init();
	slist_append(&cfg->disabled, cfg_disabled_always("head0"));

	head_apply_toggles(&head, cfg);

	assert_true(head.overrided_enabled == OverrideFalse);
	assert_log(INFO, "\nApplying \"DISABLED\" override for head0\n");
	assert_logs_empty();

	head_apply_toggles(&head, cfg);

	assert_true(head.overrided_enabled == NoOverride);
	assert_log(INFO, "\nResetting \"DISABLED\" override for head0\n");
	assert_logs_empty();

	cfg_free(cfg);
}

static void head_set_description__nulls(void **state) {
	struct Head head = { .description = strdup("orig"), };

	head_set_description(&head, "(null) (null) (null) foo (null) bar baz");

	assert_str_equal(head.description, "foo (null) bar baz");

	free(head.description);
}

static void head_set_description__no_nulls(void **state) {
	struct Head head = { .description = strdup("orig"), };

	head_set_description(&head, "foo");

	assert_str_equal(head.description, "foo");

	free(head.description);
}

static void head_set_description__empty(void **state) {
	struct Head head = { .description = strdup("orig"), };

	head_set_description(&head, "");

	assert_str_equal(head.description, "");

	free(head.description);
}

static void head_set_description__null_input(void **state) {
	struct Head head = { .description = strdup("orig"), };

	head_set_description(&head, NULL);

	assert_nul(head.description);
}

static void heads_reapply__(void **state) {
	struct SList *heads = NULL;


	struct Head *head_disabled = calloc(1, sizeof(struct Head));
	head_disabled->name = strdup("DP-7");
	head_disabled->current.enabled = false;

	slist_append(&head_disabled->modes, mode_init(NULL, NULL, 3440, 1440, 59999, true));
	slist_append(&head_disabled->modes, mode_init(NULL, NULL, 3840, 2160, 30000, false));
	slist_append(&head_disabled->modes, mode_init(NULL, NULL, 3840, 2160, 29970, false));
	slist_append(&head_disabled->modes_failed, slist_at(head_disabled->modes, 0));
	slist_append(&head_disabled->modes_failed, slist_at(head_disabled->modes, 1));
	slist_append(&head_disabled->modes_failed, slist_at(head_disabled->modes, 2));

	slist_append(&heads, head_disabled);


	struct Head *head_enabled = calloc(1, sizeof(struct Head));
	head_enabled->name = strdup("eDP-1");
	head_enabled->current.enabled = true;

	slist_append(&head_enabled->modes, mode_init(NULL, NULL, 2256, 1504, 59999, true));
	head_enabled->current.mode = slist_at(head_enabled->modes, 0);

	slist_append(&heads, head_enabled);


	heads_reapply(heads);


	char *expected_log = read_file("tst/head/reapply.log");
	assert_log(INFO, expected_log);
	assert_logs_empty();
	free(expected_log);


	slist_free_vals(&heads, head_free);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(head_get_fixed_scale__rounding_nearest),
		TEST(head_get_fixed_scale__rounding_up),
		TEST(head_get_fixed_scale__rounding_down),

		TEST(head_auto_scale__default),
		TEST(head_auto_scale__mode),
		TEST(head_auto_scale__range),

		TEST(head_set_scaled_dimensions__default),
		TEST(head_set_scaled_dimensions__transform),
		TEST(head_set_scaled_dimensions__dimensions),

		TEST(head_find_mode__all_failed),
		TEST(head_find_mode__user_available),
		TEST(head_find_mode__user_failed),
		TEST(head_find_mode__preferred),
		TEST(head_find_mode__max_preferred_refresh),
		TEST(head_find_mode__max),
		TEST(head_find_mode__none),

		TEST(head_apply_toggles__none),
		TEST(head_apply_toggles__disabled__enable),
		TEST(head_apply_toggles__disabled__disable),

		TEST(head_set_description__nulls),
		TEST(head_set_description__no_nulls),
		TEST(head_set_description__empty),
		TEST(head_set_description__null_input),

		TEST(heads_reapply__),
	};

	return RUN(tests);
}

