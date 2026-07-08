#include "tst.h"

#include "assert-head.h"
#include "assert-log.h"
#include "assert-mode.h"
#include "assert-wl.h"
#include "expects.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "cfg.h"
#include "fn.h"
#include "head.h"
#include "mode.h"
#include "pset.h"
#include "slist.h"
#include "smapi.h"
#include "sset.h"
#include "wlr-output-management-unstable-v1.h"

#include "desire.h"

extern int g_cancellation_retries;

struct State {
	struct SList *heads;
};

static int before_each(void **state) {
	g_cfg = cfg_default();

	struct State *s = calloc(1, sizeof(struct State));

	for (int i = 0; i < 10; i++) {
		struct Head *head = head_init();
		head->desired.enabled = true;
		const struct Mode *mode = mode_init_h_whr(head, i * 20, i * 10, 0);
		head->desired.mode = mode;
		pset_add(head->modes, mode);
		slist_append(&s->heads, head);
	}

	*state = s;
	return 0;
}

static int after_each(void **state) {
	assert_logs_empty();

	slist_free_vals(&g_heads, (fn_free)head_free);

	cfg_destroy();

	struct State *s = *state;

	slist_free_vals(&s->heads, (fn_free)head_free);

	free(s);
	return 0;
}

static void desire__nothing(void **state) {
	desire();
}

static void desire_order__exact_partial_regex(void **state) {
	const struct SSet *order_name_desc = sset_init();
	struct SList *heads = NULL;
	struct SList *expected = NULL;

	// ORDER
	sset_add(order_name_desc, "exact0");
	sset_add(order_name_desc, "exact1");
	sset_add(order_name_desc, "!.*regex.*");
	sset_add(order_name_desc, "exact1"); // should not repeat
	sset_add(order_name_desc, "partial");

	// heads
	struct Head *not_specified_1 = head_init_description("not specified 1");
	struct Head *exact0_partial  = head_init_description("not an exact0 exact match");
	struct Head *partial         = head_init_description("a partial match");
	struct Head *regex_match_1   = head_init_description("a regex match");
	struct Head *exact1          = head_init_description("exact1");
	struct Head *exact0          = head_init_description("exact0");
	struct Head *regex_match_2   = head_init_description("another regex match");
	struct Head *not_specified_2 = head_init_description("not specified 2");
	slist_append(&heads, not_specified_1);
	slist_append(&heads, exact0_partial);
	slist_append(&heads, partial);
	slist_append(&heads, regex_match_1);
	slist_append(&heads, exact1);
	slist_append(&heads, exact0);
	slist_append(&heads, regex_match_2);
	slist_append(&heads, not_specified_2);

	// expected
	slist_append(&expected, exact0);
	slist_append(&expected, exact0_partial);
	slist_append(&expected, exact1);
	slist_append(&expected, regex_match_1);
	slist_append(&expected, regex_match_2);
	slist_append(&expected, partial);
	slist_append(&expected, not_specified_1);
	slist_append(&expected, not_specified_2);

	struct SList *heads_ordered = desire_order(order_name_desc, heads);

	assert_heads_order(heads_ordered, expected);

	sset_free(order_name_desc);
	slist_free_vals(&heads, (fn_free)head_free);
	slist_free(&expected);
	slist_free(&heads_ordered);
}

static void desire_order__exact_regex_catchall(void **state) {
	const struct SSet *order_name_desc = sset_init();
	struct SList *heads = NULL;
	struct SList *expected = NULL;

	// ORDER
	sset_add(order_name_desc, "exact0");
	sset_add(order_name_desc, "!.*regex.*");
	sset_add(order_name_desc, "!.*$");
	sset_add(order_name_desc, "exact9");

	// heads
	struct Head *exact9          = head_init_description("exact9");
	struct Head *not_specified_1 = head_init_description("not specified 1");
	struct Head *regex_match_1   = head_init_description("a regex match");
	struct Head *exact0          = head_init_description("exact0");
	struct Head *regex_match_2   = head_init_description("another regex match");
	struct Head *not_specified_2 = head_init_description("not specified 2");
	slist_append(&heads, not_specified_1);
	slist_append(&heads, regex_match_1);
	slist_append(&heads, exact0);
	slist_append(&heads, regex_match_2);
	slist_append(&heads, not_specified_2);
	slist_append(&heads, exact9);

	// expected
	slist_append(&expected, exact0);
	slist_append(&expected, regex_match_1);
	slist_append(&expected, regex_match_2);
	slist_append(&expected, not_specified_1);
	slist_append(&expected, not_specified_2);
	slist_append(&expected, exact9);

	struct SList *heads_ordered = desire_order(order_name_desc, heads);

	assert_heads_order(heads_ordered, expected);

	sset_free(order_name_desc);
	slist_free_vals(&heads, (fn_free)head_free);
	slist_free(&expected);
	slist_free(&heads_ordered);
}

static void desire_order__no_order(void **state) {
	struct SList *heads = NULL;
	struct Head *head = head_init_name("head");

	slist_append(&heads, head);

	// null/empty order
	struct SList *heads_ordered = desire_order(NULL, heads);
	assert_heads_order(heads_ordered, heads);

	slist_free(&heads_ordered);
	slist_free_vals(&heads, (fn_free)head_free);
}

static void desire_position__col_left(void **state) {
	struct State *s = *state;
	struct Head *head;

	g_cfg->arrange = COL;
	g_cfg->align = LEFT;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 3;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	desire_positions(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 0, 0);
	head = slist_at(s->heads, 1); assert_head_position(head, 0, 2);
	head = slist_at(s->heads, 2); assert_head_position(head, 0, 5);
}

static void desire_position__col_mid(void **state) {
	struct State *s = *state;
	struct Head *head;

	g_cfg->arrange = COL;
	g_cfg->align = MIDDLE;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 3;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	desire_positions(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 2, 0);
	head = slist_at(s->heads, 1); assert_head_position(head, 0, 2);
	head = slist_at(s->heads, 2); assert_head_position(head, 3, 5);
}

static void desire_position__col_right(void **state) {
	struct State *s = *state;
	struct Head *head;

	g_cfg->arrange = COL;
	g_cfg->align = RIGHT;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 3;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	desire_positions(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 3, 0);
	head = slist_at(s->heads, 1); assert_head_position(head, 0, 2);
	head = slist_at(s->heads, 2); assert_head_position(head, 5, 5);
}

static void desire_position__row_top(void **state) {
	struct State *s = *state;
	struct Head *head;

	g_cfg->arrange = ROW;
	g_cfg->align = TOP;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 5;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	desire_positions(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 0, 0);
	head = slist_at(s->heads, 1); assert_head_position(head, 4, 0);
	head = slist_at(s->heads, 2); assert_head_position(head, 11, 0);
}

static void desire_position__row_mid(void **state) {
	struct State *s = *state;
	struct Head *head;

	g_cfg->arrange = ROW;
	g_cfg->align = MIDDLE;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 5;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	desire_positions(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 0, 2);
	head = slist_at(s->heads, 1); assert_head_position(head, 4, 0);
	head = slist_at(s->heads, 2); assert_head_position(head, 11, 2);
}

static void desire_position__row_bottom(void **state) {
	struct State *s = *state;
	struct Head *head;

	g_cfg->arrange = ROW;
	g_cfg->align = BOTTOM;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 5;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	desire_positions(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 0, 3);
	head = slist_at(s->heads, 1); assert_head_position(head, 4, 0);
	head = slist_at(s->heads, 2); assert_head_position(head, 11, 4);
}

static void desire_enabled__disabled(void **state) {
	struct Head *head = head_init();
	head->name = strdup("head0");
	head->desired.enabled = true;
	slist_append(&g_heads, head);

	expect_str(__wrap_lid_is_closed, name, "head0");
	will_return_int(__wrap_lid_is_closed, false);

	pset_add(g_cfg->disableds, disabled_init_name_desc("head0"));

	desire_enabled(head);

	assert_false(head->desired.enabled);
}

static void desire_enabled__lid_closed_many(void **state) {
	struct Head *head0 = head_init_name("head0");
	slist_append(&g_heads, head0);

	head0->desired.enabled = true;

	struct Head *head1 = head_init_name("head1");
	slist_append(&g_heads, head1);

	head1->desired.enabled = true;

	expect_str(__wrap_lid_is_closed, name, "head0");
	will_return_int(__wrap_lid_is_closed, true);

	desire_enabled(head0);

	assert_false(head0->desired.enabled);
}

static void desire_enabled__lid_closed_one(void **state) {
	struct Head *head = head_init_name("head");
	slist_append(&g_heads, head);

	head->desired.enabled = true;

	expect_str(__wrap_lid_is_closed, name, "head");
	will_return_int(__wrap_lid_is_closed, true);

	desire_enabled(head);

	assert_true(head->desired.enabled);
}

static void desire_enabled__lid_closed_one_disabled(void **state) {
	struct Head *head = head_init_name("head0");
	slist_append(&g_heads, head);

	head->desired.enabled = true;

	pset_add(g_cfg->disableds, disabled_init_name_desc("![hH]ead[0-9]"));

	expect_str(__wrap_lid_is_closed, name, "head0");
	will_return_int(__wrap_lid_is_closed, true);

	desire_enabled(head);

	assert_false(head->desired.enabled);
}

static void desire_enabled__override(void **state) {
	struct Head *head = head_init_name("head0");
	slist_append(&g_heads, head);

	head->desired.enabled = false;
	head->overrided_enabled = OverrideTrue;

	pset_add(g_cfg->disableds, disabled_init_name_desc("![hH]ead[0-9]"));

	expect_str(__wrap_lid_is_closed, name, "head0");
	will_return_int(__wrap_lid_is_closed, false);

	desire_enabled(head);

	assert_true(head->desired.enabled);
	assert_true(head->overrided_enabled == OverrideTrue);
}

static void desire_enabled__override_reset(void **state) {
	struct Head *head = head_init_name("head0");
	slist_append(&g_heads, head);

	head->desired.enabled = true;
	head->overrided_enabled = OverrideFalse;

	pset_add(g_cfg->disableds, disabled_init_name_desc("![hH]ead[0-9]"));

	expect_str(__wrap_lid_is_closed, name, "head0");
	will_return_int(__wrap_lid_is_closed, false);

	desire_enabled(head);

	assert_false(head->desired.enabled);
	assert_true(head->overrided_enabled == NoOverride);
}

static void desire_enabled__no_override(void **state) {
	struct Head *head = head_init_name("head");
	slist_append(&g_heads, head);

	head->desired.enabled = false;
	head->overrided_enabled = OverrideFalse;

	expect_str(__wrap_lid_is_closed, name, "head");
	will_return_int(__wrap_lid_is_closed, false);

	desire_enabled(head);

	assert_false(head->desired.enabled);
	assert_true(head->overrided_enabled == OverrideFalse);
}

static void desire_mode__disabled(void **state) {
	struct Head *head = head_init_name("head");
	struct Mode *mode = mode_init();
	mode->head = head;

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
	struct Head *head = head_init_name("head");
	struct Mode *mode = mode_init();
	mode->head = head;

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
	struct Head *head = head_init_name("head");
	struct Mode *mode = mode_init();
	mode->head = head;

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
	struct Head *head = head_init_name("head");
	const struct Mode *mode0 = mode_init_h_whr(head, 1, 2, 3);

	head->desired.enabled = true;
	head->desired.mode = mode0;
	pset_add(head->modes, mode0);

	struct Mode *mode1 = mode_init_h_whr(head, 4, 5, 6);
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
	struct Head *head = head_init_name("head");
	head->desired.enabled = false;

	desire_scale(head);

	head_free(head);
}

static void desire_scale__no_scaling(void **state) {
	struct Head *head = head_init_name("head");
	head->desired.enabled = true;
	g_cfg->scaling = OFF;
	g_cfg->auto_scale = ON;

	desire_scale(head);

	assert_wl_fixed_t_equal_double(head->desired.scale, 1);

	head_free(head);
}

static void desire_scale__no_auto(void **state) {
	struct Head *head = head_init_name("head");
	head->desired.enabled = true;
	g_cfg->scaling = ON;
	g_cfg->auto_scale = OFF;

	desire_scale(head);

	assert_wl_fixed_t_equal_double(head->desired.scale, 1);

	head_free(head);
}

static void desire_scale__auto(void **state) {
	struct Head *head = head_init_name("head");
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
	struct Head *head = head_init_name("head");
	head->desired.enabled = true;

	g_cfg->scaling = ON;
	g_cfg->auto_scale = ON;

	smapi_put(g_cfg->scales, "![Hh]ea.*", 3500);
	smapi_put(g_cfg->scales, "head1", 7500);

	desire_scale(head);

	assert_wl_fixed_t_equal_double(head->desired.scale, 3.5);

	head_free(head);
}

static void desire_transform__disabled(void **state) {
	struct Head *head = head_init_name("head");
	head->desired.enabled = false;
	head->desired.transform = WL_OUTPUT_TRANSFORM_90;

	smapi_put(g_cfg->transforms, "head", WL_OUTPUT_TRANSFORM_180);

	desire_transform(head);

	assert_int_equal(head->desired.transform, WL_OUTPUT_TRANSFORM_90);

	head_free(head);
}

static void desire_transform__no_transform(void **state) {
	struct Head *head = head_init_name("head");
	head->desired.enabled = true;
	head->desired.transform = WL_OUTPUT_TRANSFORM_90;

	desire_transform(head);

	assert_int_equal(head->desired.transform, WL_OUTPUT_TRANSFORM_NORMAL);

	head_free(head);
}

static void desire_transform__user(void **state) {
	struct Head *head = head_init_name("head");
	head->desired.enabled = true;
	head->desired.transform = WL_OUTPUT_TRANSFORM_90;

	smapi_put(g_cfg->transforms, "head9", WL_OUTPUT_TRANSFORM_270);
	smapi_put(g_cfg->transforms, "head", WL_OUTPUT_TRANSFORM_180);

	desire_transform(head);

	assert_int_equal(head->desired.transform, WL_OUTPUT_TRANSFORM_180);

	head_free(head);
}

static void desire_adaptive_sync__head_disabled(void **state) {
	struct Head *head = head_init_name("head");
	head->desired.enabled = false;
	head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	desire_adaptive_sync(head);

	assert_int_equal(head->desired.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED);

	head_free(head);
}

static void desire_adaptive_sync__failed(void **state) {
	struct Head *head = head_init_name("head");
	head->desired.enabled = true;
	head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
	head->adaptive_sync_failed = true;

	desire_adaptive_sync(head);

	assert_int_equal(head->desired.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED);

	head_free(head);
}

static void desire_adaptive_sync__disabled(void **state) {
	struct Head *head = head_init_name("some head");
	head->desired.enabled = true;
	head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;

	sset_add(g_cfg->adaptive_sync_off, "!.*hea");

	desire_adaptive_sync(head);

	assert_int_equal(head->desired.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED);

	head_free(head);
}

static void desire_adaptive_sync__enabled(void **state) {
	struct Head *head = head_init_name("head");
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
	const struct Mode *mode = mode_init_h_whr(head, 200, 100, 0);
	head->desired.mode = mode;
	pset_add(head->modes, mode);

	desire_scaled_dimensions(head);
	assert_int_equal(head->scaled.width, 1);
	assert_int_equal(head->scaled.height, 1);

	head_free(head);
}

static void desire_scaled_dimensions__transform(void **state) {
	struct Head *head = head_init();

	const struct Mode *mode = mode_init_h_whr(head, 200, 100, 0);
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

	const struct Mode *mode = mode_init_h_whr(head, 3840, 2160, 0);
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
	struct Head *head = head_init_name("head");
	head->desired.enabled = true;
	head->reapply_required = true;

	desire_reapply(head);

	assert_false(head->desired.enabled);

	head_free(head);
}

static void desire_reapply__not_required(void **state) {
	struct Head *head = head_init_name("head");
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

