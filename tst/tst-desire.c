#include "tst.h"

#include "assert-head.h"
#include "assert-plist.h"
#include "assert-wl.h"
#include "data.h"
#include "expects.h"
#include "util-col.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "cfg/cfg.h"
#include "cfg/condition.h"
#include "cfg/disabled.h"
#include "displ.h"
#include "enum.h"
#include "head.h"
#include "lid.h"
#include "mode.h"
#include "plist.h"
#include "ppmap.h"
#include "pset.h"
#include "simap.h"
#include "sset.h"
#include "wlr-output-management-unstable-v1.h"

#include "desire.h"

extern int g_cancellation_retries;

static struct Head *head_init_dp(int32_t width, int32_t height) {
	struct Head *head = head_init();
	head->des.enabled = true;
	head->scaled.width = width;
	head->scaled.height = height;

	ppmap_put(head->modes, M0, mode_init());
	head->des.zmode = M0;

	return head;
}

static int before_each(void **state) {
	g_cfg = cfg_default();

	g_displ = displ_init();

	return 0;
}

static int after_each(void **state) {
	displ_free(g_displ);

	g_cfg_destroy();

	return 0;
}

static void desire__nothing(void **state) {
	desire();
}

static void desire_order__exact_partial_regex(void **state) {
	const struct Sset *order_name_desc = sset_init();
	const struct Plist *expected = head_plist_init();

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
			H0, not_specified_1,
			H1, exact0_partial,
			H2, partial,
			H3, regex_match_1,
			H4, exact1,
			H5, exact0,
			H6, regex_match_2,
			H7, not_specified_2,
			NULL);

	// expected
	plist_append_many(expected,
			exact0,
			exact0_partial,
			exact1,
			regex_match_1,
			regex_match_2,
			partial,
			not_specified_1,
			not_specified_2,
			NULL);

	const struct Plist *heads_ordered = desire_order(order_name_desc, g_displ->heads);

	assert_plist_equal(heads_ordered, expected);

	sset_free(order_name_desc);
	plist_free(expected);
	plist_free(heads_ordered);
}

static void desire_order__exact_regex_catchall(void **state) {
	const struct Sset *order_name_desc = sset_init();
	const struct Plist *expected = head_plist_init();

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
			H0, exact9,
			H1, not_specified_1,
			H2, regex_match_1,
			H3, exact0,
			H4, regex_match_2,
			H5, not_specified_2,
			NULL);

	// expected
	plist_append_many(expected,
			exact0,
			regex_match_1,
			regex_match_2,
			not_specified_1,
			not_specified_2,
			exact9,
			NULL);

	const struct Plist *heads_ordered = desire_order(order_name_desc, g_displ->heads);

	assert_plist_equal(heads_ordered, expected);

	sset_free(order_name_desc);
	plist_free(expected);
	plist_free(heads_ordered);
}

static void desire_order__no_order(void **state) {
	const struct Plist *expected = head_plist_init();

	const struct Head *head = head_n("head");

	// heads
	ppmap_put(g_displ->heads, H0, head);

	// expected
	plist_append(expected, head);

	// null/empty order
	const struct Plist *heads_ordered = desire_order(NULL, g_displ->heads);
	assert_plist_equal(heads_ordered, expected);

	plist_free(expected);
	plist_free(heads_ordered);
}

static void desire_position__col_left(void **state) {
	g_cfg->arrange = COL;
	g_cfg->align = LEFT;

	struct Head *head0 = head_init_dp(4, 2);
	struct Head *head1 = head_init_dp(7, 3);
	struct Head *head2 = head_init_dp(2, 1);

	ppmap_put_many(g_displ->heads, H0, head0, H1, head1, H2, head2, NULL);

	const struct Plist *head_list = ppmap_vals_plist(g_displ->heads);

	desire_positions(head_list);

	assert_head_position(head0, 0, 0);
	assert_head_position(head1, 0, 2);
	assert_head_position(head2, 0, 5);

	plist_free(head_list);
}

static void desire_position__col_mid(void **state) {
	g_cfg->arrange = COL;
	g_cfg->align = MIDDLE;

	struct Head *head0 = head_init_dp(4, 2);
	struct Head *head1 = head_init_dp(7, 3);
	struct Head *head2 = head_init_dp(2, 1);

	ppmap_put_many(g_displ->heads, "0", head0, "1", head1, "2", head2, NULL);

	const struct Plist *head_list = ppmap_vals_plist(g_displ->heads);

	desire_positions(head_list);

	assert_head_position(head0, 2, 0);
	assert_head_position(head1, 0, 2);
	assert_head_position(head2, 3, 5);

	plist_free(head_list);
}

static void desire_position__col_right(void **state) {
	g_cfg->arrange = COL;
	g_cfg->align = RIGHT;

	struct Head *head0 = head_init_dp(4, 2);
	struct Head *head1 = head_init_dp(7, 3);
	struct Head *head2 = head_init_dp(2, 1);

	ppmap_put_many(g_displ->heads, "0", head0, "1", head1, "2", head2, NULL);

	const struct Plist *head_list = ppmap_vals_plist(g_displ->heads);

	desire_positions(head_list);

	assert_head_position(head0, 3, 0);
	assert_head_position(head1, 0, 2);
	assert_head_position(head2, 5, 5);

	plist_free(head_list);
}

static void desire_position__row_top(void **state) {
	g_cfg->arrange = ROW;
	g_cfg->align = TOP;

	struct Head *head0 = head_init_dp(4, 2);
	struct Head *head1 = head_init_dp(7, 5);
	struct Head *head2 = head_init_dp(2, 1);

	ppmap_put_many(g_displ->heads, "0", head0, "1", head1, "2", head2, NULL);

	const struct Plist *head_list = ppmap_vals_plist(g_displ->heads);

	desire_positions(head_list);

	assert_head_position(head0, 0, 0);
	assert_head_position(head1, 4, 0);
	assert_head_position(head2, 11, 0);

	plist_free(head_list);
}

static void desire_position__row_mid(void **state) {
	g_cfg->arrange = ROW;
	g_cfg->align = MIDDLE;

	struct Head *head0 = head_init_dp(4, 2);
	struct Head *head1 = head_init_dp(7, 5);
	struct Head *head2 = head_init_dp(2, 1);

	ppmap_put_many(g_displ->heads, "0", head0, "1", head1, "2", head2, NULL);

	const struct Plist *head_list = ppmap_vals_plist(g_displ->heads);

	desire_positions(head_list);

	assert_head_position(head0, 0, 2);
	assert_head_position(head1, 4, 0);
	assert_head_position(head2, 11, 2);

	plist_free(head_list);
}

static void desire_position__row_bottom(void **state) {
	g_cfg->arrange = ROW;
	g_cfg->align = BOTTOM;

	struct Head *head0 = head_init_dp(4, 2);
	struct Head *head1 = head_init_dp(7, 5);
	struct Head *head2 = head_init_dp(2, 1);

	ppmap_put_many(g_displ->heads,
			H0, head0,
			H1, head1,
			H2, head2,
			NULL);

	const struct Plist *head_list = ppmap_vals_plist(g_displ->heads);

	desire_positions(head_list);

	assert_head_position(head0, 0, 3);
	assert_head_position(head1, 4, 0);
	assert_head_position(head2, 11, 4);

	plist_free(head_list);
}

static void desire_enabled__disabled(void **state) {
	struct Head *head = head_init();
	head->name = strdup("head0");
	head->des.enabled = true;
	ppmap_put(g_displ->heads, H0, head);

	expect_str(__wrap_g_lid_is_closed, name, "head0");
	will_return_int(__wrap_g_lid_is_closed, false);

	pset_add(g_cfg->disableds, disabled_nd("head0"));

	desire_enabled(head);

	assert_false(head->des.enabled);
}

static void desire_enabled__disabled_condition(void **state) {
	g_lid = calloc(1, sizeof(struct Lid));
	g_lid->closed = true;

	expect_str_count(__wrap_g_lid_is_closed, name, "head0", EXPECT_ALWAYS);
	will_return_int_count(__wrap_g_lid_is_closed, false, EXPECT_ALWAYS);

	struct Head *head = head_n("head0");
	ppmap_put(g_displ->heads, H0, head);

	head->des.enabled = true;

	// compound condition
	struct CfgDisabled *disabled = cfg_disabled_init();
	disabled->name_desc = strdup("head0");
	struct CfgCondition *cond = cfg_condition_init();
	sset_add_many(cond->plugged, "headp1", "headp2", NULL);
	sset_add_many(cond->unplugged, "headu1", "headu2", NULL);
	cond->lid = LID_OPEN;
	pset_add(disabled->conditions, cond);

	pset_add(g_cfg->disableds, disabled);

	// unplugged
	desire_enabled(head);
	assert_true(head->des.enabled);

	// unplugged, lid
	g_lid->closed = false;
	desire_enabled(head);
	assert_true(head->des.enabled);

	// unplugged, lid
	ppmap_put(g_displ->heads, H1, head_n("headp1"));
	desire_enabled(head);
	assert_true(head->des.enabled);

	// MET: unplugged, lid, plugged
	ppmap_put(g_displ->heads, H2, head_n("headp2"));
	desire_enabled(head);
	assert_false(head->des.enabled);

	// plugged, lid
	ppmap_put(g_displ->heads, H3, head_n("headu1"));
	desire_enabled(head);
	assert_true(head->des.enabled);

	// plugged, lid
	ppmap_put_free(g_displ->heads, H4, head_n("headu2"));
	desire_enabled(head);
	assert_true(head->des.enabled);

	// MET: plugged, lid, unplugged
	ppmap_put_free(g_displ->heads, H3, head_n("headx"));
	ppmap_put_free(g_displ->heads, H4, head_n("heady"));
	desire_enabled(head);
	assert_false(head->des.enabled);

	// plugged, unplugged
	g_lid->closed = true;
	desire_enabled(head);
	assert_true(head->des.enabled);

	free(g_lid);
}

static void desire_enabled__lid_closed_many(void **state) {
	struct Head *head0 = head_n("head0");
	ppmap_put(g_displ->heads, H0, head0);

	head0->des.enabled = true;

	struct Head *head1 = head_n("head1");
	ppmap_put(g_displ->heads, H1, head1);

	head1->des.enabled = true;

	expect_str(__wrap_g_lid_is_closed, name, "head0");
	will_return_int(__wrap_g_lid_is_closed, true);

	desire_enabled(head0);

	assert_false(head0->des.enabled);
}

static void desire_enabled__lid_closed_one(void **state) {
	struct Head *head = head_n("head");
	ppmap_put(g_displ->heads, H0, head);

	head->des.enabled = true;

	expect_str(__wrap_g_lid_is_closed, name, "head");
	will_return_int(__wrap_g_lid_is_closed, true);

	desire_enabled(head);

	assert_true(head->des.enabled);
}

static void desire_enabled__lid_closed_one_disabled(void **state) {
	struct Head *head = head_n("head0");
	ppmap_put(g_displ->heads, H0, head);

	head->des.enabled = true;

	pset_add(g_cfg->disableds, disabled_nd("![hH]ead[0-9]"));

	expect_str(__wrap_g_lid_is_closed, name, "head0");
	will_return_int(__wrap_g_lid_is_closed, true);

	desire_enabled(head);

	assert_false(head->des.enabled);
}

static void desire_enabled__override(void **state) {
	struct Head *head = head_n("head0");
	ppmap_put(g_displ->heads, H0, head);

	head->des.enabled = false;
	head->overrided_enabled = OverrideTrue;

	pset_add(g_cfg->disableds, disabled_nd("![hH]ead[0-9]"));

	expect_str(__wrap_g_lid_is_closed, name, "head0");
	will_return_int(__wrap_g_lid_is_closed, false);

	desire_enabled(head);

	assert_true(head->des.enabled);
	assert_true(head->overrided_enabled == OverrideTrue);
}

static void desire_enabled__override_reset(void **state) {
	struct Head *head = head_n("head0");
	ppmap_put(g_displ->heads, H0, head);

	head->des.enabled = true;
	head->overrided_enabled = OverrideFalse;

	pset_add(g_cfg->disableds, disabled_nd("![hH]ead[0-9]"));

	expect_str(__wrap_g_lid_is_closed, name, "head0");
	will_return_int(__wrap_g_lid_is_closed, false);

	desire_enabled(head);

	assert_false(head->des.enabled);
	assert_true(head->overrided_enabled == NoOverride);
}

static void desire_enabled__no_override(void **state) {
	struct Head *head = head_n("head");
	ppmap_put(g_displ->heads, H0, head);

	head->des.enabled = false;
	head->overrided_enabled = OverrideFalse;

	expect_str(__wrap_g_lid_is_closed, name, "head");
	will_return_int(__wrap_g_lid_is_closed, false);

	desire_enabled(head);

	assert_false(head->des.enabled);
	assert_true(head->overrided_enabled == OverrideFalse);
}

static void desire_mode__disabled(void **state) {
	struct Head *head = head_n("head");

	ppmap_put(head->modes, M0, mode_init());
	head->des.enabled = false;
	head->des.zmode = M0;

	desire_mode(head);

	assert_ptr_equal(head->des.zmode, M0);
	assert_false(head->des.enabled);
	assert_false(head->warned_no_mode);

	head_free(head);
}

static void desire_mode__no_mode(void **state) {
	struct Head *head = head_n("head");

	ppmap_put(head->modes, M0, mode_init());
	head->des.enabled = true;
	head->des.zmode = M0;

	expect_ptr(__wrap_head_find_mode, head, head);
	will_return_ptr_type(__wrap_head_find_mode, NULL, struct zwlr_output_mode_v1*);

	desire_mode(head);

	assert_ptr_equal(head->des.zmode, M0);
	assert_false(head->des.enabled);
	assert_true(head->warned_no_mode);

	head_free(head);
}

static void desire_mode__no_mode_warned(void **state) {
	struct Head *head = head_n("head");

	ppmap_put(head->modes, MD, mode_init());
	head->des.enabled = true;
	head->des.zmode = MD;

	head->warned_no_mode = false;

	expect_ptr(__wrap_head_find_mode, head, head);
	will_return_ptr_type(__wrap_head_find_mode, NULL, struct zwlr_output_mode_v1*);

	desire_mode(head);

	assert_ptr_equal(head->des.zmode, MD);
	assert_false(head->des.enabled);
	assert_true(head->warned_no_mode);

	head_free(head);
}

static void desire_mode__ok(void **state) {
	struct Head *head = head_n("head");

	ppmap_put_many(head->modes,
			M0, mode_whr(1, 2, 3),
			MD, mode_whr(4, 5, 6),
			NULL
			);

	head->des.enabled = true;

	expect_ptr(__wrap_head_find_mode, head, head);
	will_return_ptr_type(__wrap_head_find_mode, MD, struct zwlr_output_mode_v1*);

	desire_mode(head);

	assert_ptr_equal(head->des.zmode, MD);
	assert_true(head->des.enabled);
	assert_false(head->warned_no_mode);

	head_free(head);
}

static void desire_scale__disabled(void **state) {
	struct Head *head = head_n("head");
	head->des.enabled = false;

	desire_scale(head);

	head_free(head);
}

static void desire_scale__no_scaling(void **state) {
	struct Head *head = head_n("head");
	head->des.enabled = true;
	g_cfg->scaling = OFF;
	g_cfg->auto_scale = ON;

	desire_scale(head);

	assert_wl_fixed_t_equal_double(head->des.scale, 1);

	head_free(head);
}

static void desire_scale__no_auto(void **state) {
	struct Head *head = head_n("head");
	head->des.enabled = true;
	g_cfg->scaling = ON;
	g_cfg->auto_scale = OFF;

	desire_scale(head);

	assert_wl_fixed_t_equal_double(head->des.scale, 1);

	head_free(head);
}

static void desire_scale__auto(void **state) {
	struct Head *head = head_n("head");
	head->des.enabled = true;

	g_cfg->scaling = ON;
	g_cfg->auto_scale = ON;

	expect_ptr(__wrap_head_auto_scale, head, head);
	will_return_int(__wrap_head_auto_scale, wl_fixed_from_double(2.5));

	desire_scale(head);

	assert_wl_fixed_t_equal_double(head->des.scale, 2.5);

	head_free(head);
}

static void desire_scale__user(void **state) {
	struct Head *head = head_n("head");
	head->des.enabled = true;

	g_cfg->scaling = ON;
	g_cfg->auto_scale = ON;

	simap_put(g_cfg->scales,"![Hh]ea.*", 3500);
	simap_put(g_cfg->scales,"head1",     7500);

	desire_scale(head);

	assert_wl_fixed_t_equal_double(head->des.scale, 3.5);

	head_free(head);
}

static void desire_transform__disabled(void **state) {
	struct Head *head = head_n("head");
	head->des.enabled = false;
	head->des.transform = WL_OUTPUT_TRANSFORM_90;

	simap_put(g_cfg->transforms, "head", WL_OUTPUT_TRANSFORM_180);

	desire_transform(head);

	assert_int_equal(head->des.transform, WL_OUTPUT_TRANSFORM_90);

	head_free(head);
}

static void desire_transform__no_transform(void **state) {
	struct Head *head = head_n("head");
	head->des.enabled = true;
	head->des.transform = WL_OUTPUT_TRANSFORM_90;

	desire_transform(head);

	assert_int_equal(head->des.transform, WL_OUTPUT_TRANSFORM_NORMAL);

	head_free(head);
}

static void desire_transform__user(void **state) {
	struct Head *head = head_n("head");
	head->des.enabled = true;
	head->des.transform = WL_OUTPUT_TRANSFORM_90;

	simap_put(g_cfg->transforms, "head9", WL_OUTPUT_TRANSFORM_270);
	simap_put(g_cfg->transforms, "head",  WL_OUTPUT_TRANSFORM_180);

	desire_transform(head);

	assert_int_equal(head->des.transform, WL_OUTPUT_TRANSFORM_180);

	head_free(head);
}

static void desire_adaptive_sync__head_disabled(void **state) {
	struct Head *head = head_n("head");
	head->des.enabled = false;
	head->des.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	desire_adaptive_sync(head);

	assert_int_equal(head->des.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED);

	head_free(head);
}

static void desire_adaptive_sync__failed(void **state) {
	struct Head *head = head_n("head");
	head->des.enabled = true;
	head->des.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
	head->adaptive_sync_failed = true;

	desire_adaptive_sync(head);

	assert_int_equal(head->des.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED);

	head_free(head);
}

static void desire_adaptive_sync__disabled(void **state) {
	struct Head *head = head_n("some head");
	head->des.enabled = true;
	head->des.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;

	sset_add(g_cfg->adaptive_sync_off, "!.*hea");

	desire_adaptive_sync(head);

	assert_int_equal(head->des.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED);

	head_free(head);
}

static void desire_adaptive_sync__enabled(void **state) {
	struct Head *head = head_n("head");
	head->des.enabled = true;
	head->des.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	desire_adaptive_sync(head);

	assert_int_equal(head->des.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED);

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
	ppmap_put(head->modes, M0, mode_whr(200, 100, 0));

	desire_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 1);
	assert_int_equal(head->scaled.height, 1);

	head_free(head);
}

static void desire_scaled_dimensions__transform(void **state) {
	struct Head *head = head_init();

	ppmap_put(head->modes, M0, mode_whr(200, 100, 0));
	head->des.zmode = M0;

	// double, not rotated
	head->des.scale = wl_fixed_from_double(0.5);
	head->des.transform = WL_OUTPUT_TRANSFORM_180;

	desire_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 400);
	assert_int_equal(head->scaled.height, 200);

	// one third, rotated
	head->des.scale = wl_fixed_from_double(3);
	head->des.transform = WL_OUTPUT_TRANSFORM_90;

	desire_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 33);
	assert_int_equal(head->scaled.height, 66); // wayland truncates when calculating size

	head_free(head);
}

static void desire_scaled_dimensions__dimensions(void **state) {
	struct Head *head = head_init();

	ppmap_put(head->modes, M0, mode_whr(3840, 2160, 0));
	head->des.zmode = M0;

	head->des.scale = head_get_fixed_scale(1.0);
	desire_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 3840);
	assert_int_equal(head->scaled.height, 2160);

	head->des.scale = head_get_fixed_scale(2.0);
	desire_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 1920);
	assert_int_equal(head->scaled.height, 1080);

	head->des.scale = head_get_fixed_scale(1.7);
	// actual scale will be 1.75
	desire_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 2194);
	assert_int_equal(head->scaled.height, 1234);

	head->des.scale = head_get_fixed_scale(1.9);
	// actual scale will be 1.875
	desire_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 2048);
	assert_int_equal(head->scaled.height, 1152);

	head->name = strdup("name");

	head->des.scale = head_get_fixed_scale(2.01);
	// actual scale will be 2.0
	desire_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 1920);
	assert_int_equal(head->scaled.height, 1080);

	head_free(head);
}


static void desire_reapply__required(void **state) {
	struct Head *head = head_n("head");
	head->des.enabled = true;
	head->reapply_required = true;

	desire_reapply(head);

	assert_false(head->des.enabled);

	head_free(head);
}

static void desire_reapply__not_required(void **state) {
	struct Head *head = head_n("head");
	head->des.enabled = true;
	head->reapply_required = false;

	desire_reapply(head);

	assert_true(head->des.enabled);

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
		TEST_BA(desire_enabled__disabled_condition),
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

