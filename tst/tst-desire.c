#include "tst.h"

#include "assert-head.h"
#include "assert-log.h"
#include "assert-mode.h"
#include "assert-wl.h"
#include "expects.h"
#include "util-col.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "cfg/cfg.h"
#include "displ.h"
#include "enum.h"
#include "head.h"
#include "mode.h"
#include "ppmap.h"
#include "pset.h"
#include "simap.h"
#include "sset.h"
#include "wlr-output-management-unstable-v1.h"

#include "desire.h"

extern int g_cancellation_retries;

static struct Head *head_init_dp(int32_t width, int32_t height) {
	struct Head *head = head_init();
	head->desired.enabled = true;
	head->scaled.width = width;
	head->scaled.height = height;
	head->desired.mode = mode_init();
	return head;
}

static int before_each(void **state) {
	g_cfg = cfg_default();

	g_displ = displ_init();

	return 0;
}

static int after_each(void **state) {
	assert_logs_empty();

	displ_free(g_displ);

	g_cfg_destroy();

	return 0;
}

static void desire__nothing(void **state) {
	desire();
}

static void desire_order__exact_partial_regex(void **state) {
	const struct Sset *order_name_desc = sset_init();
	const struct Pset *expected = head_pset_init();

	// ORDER
	sset_add_many(order_name_desc,
			"exact0",
			"exact1",
			"!.*regex.*",
			"exact1", // should not repeat
			"partial",
			NULL);

	// heads
	struct Head *not_specified_1 = head_d("not specified 1");
	struct Head *exact0_partial  = head_d("not an exact0 exact match");
	struct Head *partial         = head_d("a partial match");
	struct Head *regex_match_1   = head_d("a regex match");
	struct Head *exact1          = head_d("exact1");
	struct Head *exact0          = head_d("exact0");
	struct Head *regex_match_2   = head_d("another regex match");
	struct Head *not_specified_2 = head_d("not specified 2");
	ppmap_put_many(g_displ->heads,
			"0", not_specified_1,
			"1", exact0_partial,
			"2", partial,
			"3", regex_match_1,
			"4", exact1,
			"5", exact0,
			"6", regex_match_2,
			"7", not_specified_2,
			NULL);

	// expected
	pset_add_many(expected,
			exact0,
			exact0_partial,
			exact1,
			regex_match_1,
			regex_match_2,
			partial,
			not_specified_1,
			not_specified_2,
			NULL);

	const struct Pset *heads_ordered = desire_order(order_name_desc, g_displ->heads);

	assert_heads_order(heads_ordered, expected);

	sset_free(order_name_desc);
	pset_free(expected);
	pset_free(heads_ordered);
}

static void desire_order__exact_regex_catchall(void **state) {
	const struct Sset *order_name_desc = sset_init();
	const struct Pset *expected = head_pset_init();

	// ORDER
	sset_add_many(order_name_desc,
			"exact0",
			"!.*regex.*",
			"!.*$",
			"exact9",
			NULL);

	// heads
	struct Head *exact9          = head_d("exact9");
	struct Head *not_specified_1 = head_d("not specified 1");
	struct Head *regex_match_1   = head_d("a regex match");
	struct Head *exact0          = head_d("exact0");
	struct Head *regex_match_2   = head_d("another regex match");
	struct Head *not_specified_2 = head_d("not specified 2");
	ppmap_put_many(g_displ->heads,
			"0", exact9,
			"1", not_specified_1,
			"2", regex_match_1,
			"3", exact0,
			"4", regex_match_2,
			"5", not_specified_2,
			NULL);

	// expected
	pset_add_many(expected,
			exact0,
			regex_match_1,
			regex_match_2,
			not_specified_1,
			not_specified_2,
			exact9,
			NULL);

	const struct Pset *heads_ordered = desire_order(order_name_desc, g_displ->heads);

	assert_heads_order(heads_ordered, expected);

	sset_free(order_name_desc);
	pset_free(expected);
	pset_free(heads_ordered);
}

static void desire_order__no_order(void **state) {
	const struct Pset *expected = head_pset_init();

	const struct Head *head = head_n("head");

	// heads
	ppmap_put(g_displ->heads, "0", head);

	// expected
	pset_add(expected, head);

	// null/empty order
	const struct Pset *heads_ordered = desire_order(NULL, g_displ->heads);
	assert_heads_order(heads_ordered, expected);

	pset_free(expected);
	pset_free(heads_ordered);
}

static void desire_position__col_left(void **state) {
	g_cfg->arrange = COL;
	g_cfg->align = LEFT;

	struct Head *head0 = head_init_dp(4, 2);
	struct Head *head1 = head_init_dp(7, 3);
	struct Head *head2 = head_init_dp(2, 1);

	ppmap_put_many(g_displ->heads, "0", head0, "1", head1, "2", head2, NULL);

	const struct Pset *head_set = ppmap_vals_pset(g_displ->heads);

	desire_positions(head_set);

	assert_head_position(head0, 0, 0);
	assert_head_position(head1, 0, 2);
	assert_head_position(head2, 0, 5);

	pset_free(head_set);
}

static void desire_position__col_mid(void **state) {
	g_cfg->arrange = COL;
	g_cfg->align = MIDDLE;

	struct Head *head0 = head_init_dp(4, 2);
	struct Head *head1 = head_init_dp(7, 3);
	struct Head *head2 = head_init_dp(2, 1);

	ppmap_put_many(g_displ->heads, "0", head0, "1", head1, "2", head2, NULL);

	const struct Pset *head_set = ppmap_vals_pset(g_displ->heads);

	desire_positions(head_set);

	assert_head_position(head0, 2, 0);
	assert_head_position(head1, 0, 2);
	assert_head_position(head2, 3, 5);

	pset_free(head_set);
}

static void desire_position__col_right(void **state) {
	g_cfg->arrange = COL;
	g_cfg->align = RIGHT;

	struct Head *head0 = head_init_dp(4, 2);
	struct Head *head1 = head_init_dp(7, 3);
	struct Head *head2 = head_init_dp(2, 1);

	ppmap_put_many(g_displ->heads, "0", head0, "1", head1, "2", head2, NULL);

	const struct Pset *head_set = ppmap_vals_pset(g_displ->heads);

	desire_positions(head_set);

	assert_head_position(head0, 3, 0);
	assert_head_position(head1, 0, 2);
	assert_head_position(head2, 5, 5);

	pset_free(head_set);
}

static void desire_position__row_top(void **state) {
	g_cfg->arrange = ROW;
	g_cfg->align = TOP;

	struct Head *head0 = head_init_dp(4, 2);
	struct Head *head1 = head_init_dp(7, 5);
	struct Head *head2 = head_init_dp(2, 1);

	ppmap_put_many(g_displ->heads, "0", head0, "1", head1, "2", head2, NULL);

	const struct Pset *head_set = ppmap_vals_pset(g_displ->heads);

	desire_positions(head_set);

	assert_head_position(head0, 0, 0);
	assert_head_position(head1, 4, 0);
	assert_head_position(head2, 11, 0);

	pset_free(head_set);
}

static void desire_position__row_mid(void **state) {
	g_cfg->arrange = ROW;
	g_cfg->align = MIDDLE;

	struct Head *head0 = head_init_dp(4, 2);
	struct Head *head1 = head_init_dp(7, 5);
	struct Head *head2 = head_init_dp(2, 1);

	ppmap_put_many(g_displ->heads, "0", head0, "1", head1, "2", head2, NULL);

	const struct Pset *head_set = ppmap_vals_pset(g_displ->heads);

	desire_positions(head_set);

	assert_head_position(head0, 0, 2);
	assert_head_position(head1, 4, 0);
	assert_head_position(head2, 11, 2);

	pset_free(head_set);
}

static void desire_position__row_bottom(void **state) {
	g_cfg->arrange = ROW;
	g_cfg->align = BOTTOM;

	struct Head *head0 = head_init_dp(4, 2);
	struct Head *head1 = head_init_dp(7, 5);
	struct Head *head2 = head_init_dp(2, 1);

	ppmap_put_many(g_displ->heads, "0", head0, "1", head1, "2", head2, NULL);

	const struct Pset *head_set = ppmap_vals_pset(g_displ->heads);

	desire_positions(head_set);

	assert_head_position(head0, 0, 3);
	assert_head_position(head1, 4, 0);
	assert_head_position(head2, 11, 4);

	pset_free(head_set);
}

static void desire_enabled__disabled(void **state) {
	struct Head *head = head_init();
	head->name = strdup("head0");
	head->desired.enabled = true;
	ppmap_put(g_displ->heads, head, head);

	expect_str(__wrap_g_lid_is_closed, name, "head0");
	will_return_int(__wrap_g_lid_is_closed, false);

	pset_add(g_cfg->disableds, disabled_nd("head0"));

	desire_enabled(head);

	assert_false(head->desired.enabled);
}

static void desire_enabled__lid_closed_many(void **state) {
	struct Head *head0 = head_n("head0");
	ppmap_put(g_displ->heads, head0, head0);

	head0->desired.enabled = true;

	struct Head *head1 = head_n("head1");
	ppmap_put(g_displ->heads, head1, head1);

	head1->desired.enabled = true;

	expect_str(__wrap_g_lid_is_closed, name, "head0");
	will_return_int(__wrap_g_lid_is_closed, true);

	desire_enabled(head0);

	assert_false(head0->desired.enabled);
}

static void desire_enabled__lid_closed_one(void **state) {
	struct Head *head = head_n("head");
	ppmap_put(g_displ->heads, head, head);

	head->desired.enabled = true;

	expect_str(__wrap_g_lid_is_closed, name, "head");
	will_return_int(__wrap_g_lid_is_closed, true);

	desire_enabled(head);

	assert_true(head->desired.enabled);
}

static void desire_enabled__lid_closed_one_disabled(void **state) {
	struct Head *head = head_n("head0");
	ppmap_put(g_displ->heads, head, head);

	head->desired.enabled = true;

	pset_add(g_cfg->disableds, disabled_nd("![hH]ead[0-9]"));

	expect_str(__wrap_g_lid_is_closed, name, "head0");
	will_return_int(__wrap_g_lid_is_closed, true);

	desire_enabled(head);

	assert_false(head->desired.enabled);
}

static void desire_enabled__override(void **state) {
	struct Head *head = head_n("head0");
	ppmap_put(g_displ->heads, head, head);

	head->desired.enabled = false;
	head->overrided_enabled = OverrideTrue;

	pset_add(g_cfg->disableds, disabled_nd("![hH]ead[0-9]"));

	expect_str(__wrap_g_lid_is_closed, name, "head0");
	will_return_int(__wrap_g_lid_is_closed, false);

	desire_enabled(head);

	assert_true(head->desired.enabled);
	assert_true(head->overrided_enabled == OverrideTrue);
}

static void desire_enabled__override_reset(void **state) {
	struct Head *head = head_n("head0");
	ppmap_put(g_displ->heads, head, head);

	head->desired.enabled = true;
	head->overrided_enabled = OverrideFalse;

	pset_add(g_cfg->disableds, disabled_nd("![hH]ead[0-9]"));

	expect_str(__wrap_g_lid_is_closed, name, "head0");
	will_return_int(__wrap_g_lid_is_closed, false);

	desire_enabled(head);

	assert_false(head->desired.enabled);
	assert_true(head->overrided_enabled == NoOverride);
}

static void desire_enabled__no_override(void **state) {
	struct Head *head = head_n("head");
	ppmap_put(g_displ->heads, head, head);

	head->desired.enabled = false;
	head->overrided_enabled = OverrideFalse;

	expect_str(__wrap_g_lid_is_closed, name, "head");
	will_return_int(__wrap_g_lid_is_closed, false);

	desire_enabled(head);

	assert_false(head->desired.enabled);
	assert_true(head->overrided_enabled == OverrideFalse);
}

static void desire_mode__disabled(void **state) {
	struct Head *head = head_n("head");
	struct Mode *mode = mode_h(head);

	head->desired.enabled = false;
	head->desired.mode = mode;
	pset_add(head->modes, mode);

	desire_mode(head);

	assert_mode_equal(head->desired.mode, mode);
	assert_ptr_equal(head->desired.mode, mode);
	assert_false(head->desired.enabled);
	assert_false(head->warned_no_mode);

	head_free(head);
}

static void desire_mode__no_mode(void **state) {
	struct Head *head = head_n("head");
	struct Mode *mode = mode_h(head);

	head->desired.enabled = true;
	head->desired.mode = mode;
	pset_add(head->modes, mode);

	expect_ptr(__wrap_head_find_mode, head, head);
	will_return_ptr_type(__wrap_head_find_mode, NULL, struct Mode*);

	desire_mode(head);

	assert_mode_equal(head->desired.mode, mode);
	assert_ptr_equal(head->desired.mode, mode);
	assert_false(head->desired.enabled);
	assert_true(head->warned_no_mode);

	head_free(head);
}

static void desire_mode__no_mode_warned(void **state) {
	struct Head *head = head_n("head");
	struct Mode *mode = mode_h(head);

	head->desired.enabled = true;
	head->desired.mode = mode;
	head->warned_no_mode = true;
	pset_add(head->modes, mode);

	expect_ptr(__wrap_head_find_mode, head, head);
	will_return_ptr_type(__wrap_head_find_mode, NULL, struct Mode*);

	desire_mode(head);

	assert_mode_equal(head->desired.mode, mode);
	assert_ptr_equal(head->desired.mode, mode);
	assert_false(head->desired.enabled);
	assert_true(head->warned_no_mode);

	head_free(head);
}

static void desire_mode__ok(void **state) {
	struct Head *head = head_n("head");
	const struct Mode *mode0 = mode_h_whr(head, 1, 2, 3);

	head->desired.enabled = true;
	head->desired.mode = mode0;
	pset_add(head->modes, mode0);

	struct Mode *mode1 = mode_h_whr(head, 4, 5, 6);
	pset_add(head->modes, mode1);

	expect_ptr(__wrap_head_find_mode, head, head);
	will_return_ptr_type(__wrap_head_find_mode, mode1, struct Mode*);

	desire_mode(head);

	assert_mode_equal(head->desired.mode, mode1);
	assert_ptr_equal(head->desired.mode, mode1);
	assert_true(head->desired.enabled);
	assert_false(head->warned_no_mode);

	head_free(head);
}

static void desire_scale__disabled(void **state) {
	struct Head *head = head_n("head");
	head->desired.enabled = false;

	desire_scale(head);

	head_free(head);
}

static void desire_scale__no_scaling(void **state) {
	struct Head *head = head_n("head");
	head->desired.enabled = true;
	g_cfg->scaling = OFF;
	g_cfg->auto_scale = ON;

	desire_scale(head);

	assert_wl_fixed_t_equal_double(head->desired.scale, 1);

	head_free(head);
}

static void desire_scale__no_auto(void **state) {
	struct Head *head = head_n("head");
	head->desired.enabled = true;
	g_cfg->scaling = ON;
	g_cfg->auto_scale = OFF;

	desire_scale(head);

	assert_wl_fixed_t_equal_double(head->desired.scale, 1);

	head_free(head);
}

static void desire_scale__auto(void **state) {
	struct Head *head = head_n("head");
	head->desired.enabled = true;

	g_cfg->scaling = ON;
	g_cfg->auto_scale = ON;

	expect_ptr(__wrap_head_auto_scale, head, head);
	will_return_int(__wrap_head_auto_scale, wl_fixed_from_double(2.5));

	desire_scale(head);

	assert_wl_fixed_t_equal_double(head->desired.scale, 2.5);

	head_free(head);
}

static void desire_scale__user(void **state) {
	struct Head *head = head_n("head");
	head->desired.enabled = true;

	g_cfg->scaling = ON;
	g_cfg->auto_scale = ON;

	simap_put_many(g_cfg->scales,
			"![Hh]ea.*", (size_t)3500,
			"head1",     (size_t)7500,
			NULL);

	desire_scale(head);

	assert_wl_fixed_t_equal_double(head->desired.scale, 3.5);

	head_free(head);
}

static void desire_transform__disabled(void **state) {
	struct Head *head = head_n("head");
	head->desired.enabled = false;
	head->desired.transform = WL_OUTPUT_TRANSFORM_90;

	simap_put(g_cfg->transforms, "head", WL_OUTPUT_TRANSFORM_180);

	desire_transform(head);

	assert_int_equal(head->desired.transform, WL_OUTPUT_TRANSFORM_90);

	head_free(head);
}

static void desire_transform__no_transform(void **state) {
	struct Head *head = head_n("head");
	head->desired.enabled = true;
	head->desired.transform = WL_OUTPUT_TRANSFORM_90;

	desire_transform(head);

	assert_int_equal(head->desired.transform, WL_OUTPUT_TRANSFORM_NORMAL);

	head_free(head);
}

static void desire_transform__user(void **state) {
	struct Head *head = head_n("head");
	head->desired.enabled = true;
	head->desired.transform = WL_OUTPUT_TRANSFORM_90;

	simap_put_many(g_cfg->transforms,
			"head9", (size_t)WL_OUTPUT_TRANSFORM_270,
			"head",  (size_t)WL_OUTPUT_TRANSFORM_180,
			NULL);

	desire_transform(head);

	assert_int_equal(head->desired.transform, WL_OUTPUT_TRANSFORM_180);

	head_free(head);
}

static void desire_adaptive_sync__head_disabled(void **state) {
	struct Head *head = head_n("head");
	head->desired.enabled = false;
	head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	desire_adaptive_sync(head);

	assert_int_equal(head->desired.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED);

	head_free(head);
}

static void desire_adaptive_sync__failed(void **state) {
	struct Head *head = head_n("head");
	head->desired.enabled = true;
	head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
	head->adaptive_sync_failed = true;

	desire_adaptive_sync(head);

	assert_int_equal(head->desired.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED);

	head_free(head);
}

static void desire_adaptive_sync__disabled(void **state) {
	struct Head *head = head_n("some head");
	head->desired.enabled = true;
	head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;

	sset_add(g_cfg->adaptive_sync_off, "!.*hea");

	desire_adaptive_sync(head);

	assert_int_equal(head->desired.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED);

	head_free(head);
}

static void desire_adaptive_sync__enabled(void **state) {
	struct Head *head = head_n("head");
	head->desired.enabled = true;
	head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	desire_adaptive_sync(head);

	assert_int_equal(head->desired.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED);

	head_free(head);
}

static void desire_scaled_dimensions__default(void **state) {
	struct Head *head = head_init();
	head->scaled.width = 1;
	head->scaled.height = 1;

	// no head
	desire_scaled_dimensions(NULL);

	// no mode
	desire_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 1);
	assert_int_equal(head->scaled.height, 1);

	// no scale
	const struct Mode *mode = mode_h_whr(head, 200, 100, 0);
	head->desired.mode = mode;
	pset_add(head->modes, mode);

	desire_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 1);
	assert_int_equal(head->scaled.height, 1);

	head_free(head);
}

static void desire_scaled_dimensions__transform(void **state) {
	struct Head *head = head_init();

	const struct Mode *mode = mode_h_whr(head, 200, 100, 0);
	head->desired.mode = mode;
	pset_add(head->modes, mode);

	// double, not rotated
	head->desired.scale = wl_fixed_from_double(0.5);
	head->desired.transform = WL_OUTPUT_TRANSFORM_180;

	desire_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 400);
	assert_int_equal(head->scaled.height, 200);

	// one third, rotated
	head->desired.scale = wl_fixed_from_double(3);
	head->desired.transform = WL_OUTPUT_TRANSFORM_90;

	desire_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 33);
	assert_int_equal(head->scaled.height, 66); // wayland truncates when calculating size

	head_free(head);
}

static void desire_scaled_dimensions__dimensions(void **state) {
	struct Head *head = head_init();

	const struct Mode *mode = mode_h_whr(head, 3840, 2160, 0);
	head->desired.mode = mode;
	pset_add(head->modes, mode);

	head->desired.scale = head_get_fixed_scale(1.0);
	desire_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 3840);
	assert_int_equal(head->scaled.height, 2160);

	head->desired.scale = head_get_fixed_scale(2.0);
	desire_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 1920);
	assert_int_equal(head->scaled.height, 1080);

	head->desired.scale = head_get_fixed_scale(1.7);
	// actual scale will be 1.75
	desire_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 2194);
	assert_int_equal(head->scaled.height, 1234);

	head->desired.scale = head_get_fixed_scale(1.9);
	// actual scale will be 1.875
	desire_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 2048);
	assert_int_equal(head->scaled.height, 1152);

	head->name = strdup("name");

	head->desired.scale = head_get_fixed_scale(2.01);
	// actual scale will be 2.0
	desire_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 1920);
	assert_int_equal(head->scaled.height, 1080);

	head_free(head);
}


static void desire_reapply__required(void **state) {
	struct Head *head = head_n("head");
	head->desired.enabled = true;
	head->reapply_required = true;

	desire_reapply(head);

	assert_false(head->desired.enabled);

	head_free(head);
}

static void desire_reapply__not_required(void **state) {
	struct Head *head = head_n("head");
	head->desired.enabled = true;
	head->reapply_required = false;

	desire_reapply(head);

	assert_true(head->desired.enabled);

	head_free(head);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(desire__nothing),

		TEST_BA(desire_order__exact_partial_regex),
		TEST_BA(desire_order__exact_regex_catchall),
		TEST_BA(desire_order__no_order),

		TEST_BA(desire_position__col_left),
		TEST_BA(desire_position__col_mid),
		TEST_BA(desire_position__col_right),
		TEST_BA(desire_position__row_top),
		TEST_BA(desire_position__row_mid),
		TEST_BA(desire_position__row_bottom),

		TEST_BA(desire_enabled__disabled),
		TEST_BA(desire_enabled__lid_closed_many),
		TEST_BA(desire_enabled__lid_closed_one_disabled),
		TEST_BA(desire_enabled__lid_closed_one),
		TEST_BA(desire_enabled__override),
		TEST_BA(desire_enabled__override_reset),
		TEST_BA(desire_enabled__no_override),

		TEST_BA(desire_mode__disabled),
		TEST_BA(desire_mode__no_mode),
		TEST_BA(desire_mode__no_mode_warned),
		TEST_BA(desire_mode__ok),

		TEST_BA(desire_scale__disabled),
		TEST_BA(desire_scale__no_scaling),
		TEST_BA(desire_scale__no_auto),
		TEST_BA(desire_scale__auto),
		TEST_BA(desire_scale__user),

		TEST_BA(desire_transform__disabled),
		TEST_BA(desire_transform__no_transform),
		TEST_BA(desire_transform__user),

		TEST_BA(desire_adaptive_sync__head_disabled),
		TEST_BA(desire_adaptive_sync__failed),
		TEST_BA(desire_adaptive_sync__disabled),
		TEST_BA(desire_adaptive_sync__enabled),

		TEST_BA(desire_scaled_dimensions__default),
		TEST_BA(desire_scaled_dimensions__transform),
		TEST_BA(desire_scaled_dimensions__dimensions),

		TEST_BA(desire_reapply__required),
		TEST_BA(desire_reapply__not_required),
	};

	return RUN(tests);
}

