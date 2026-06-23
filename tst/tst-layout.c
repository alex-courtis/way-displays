#include "tst.h"

#include "assert-head.h"
#include "assert-log.h"
#include "assert-wl.h"
#include "asserts.h"
#include "expects.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "cfg.h"
#include "cfg/disabled.h"
#include "cfg/user-scale.h"
#include "displ.h"
#include "head.h"
#include "log.h"
#include "mode.h"
#include "pset.h"
#include "slist.h"
#include "smap.h"
#include "sset.h"
#include "wlr-output-management-unstable-v1.h"

struct SList *order_heads(const struct SList *order_name_desc, struct SList *heads);
void position_heads(struct SList *heads);
void desire_enabled(struct Head *head);
void desire_mode(struct Head *head);
void desire_scale(struct Head *head);
void desire_transform(struct Head *head);
void desire_adaptive_sync(struct Head *head);
void desire_reapply(struct Head *head);
void handle_success(void);
void handle_failure(void);


// cppcheck-suppress staticFunction
struct Mode *__wrap_head_find_mode(struct Head *head) {
	check_expected_ptr(head);
	return mock_ptr_type_checked(struct Mode*);
}

// cppcheck-suppress staticFunction
wl_fixed_t __wrap_head_auto_scale(struct Head *head) {
	check_expected_ptr(head);
	return mock_type(wl_fixed_t);
}


struct State {
	struct Mode *mode;
	struct SList *heads;
};


static int before_each(void **state) {
	g_cfg = cfg_default();

	// only set this when we specifically want to test it
	free(g_cfg->callback_cmd);
	g_cfg->callback_cmd = NULL;

	g_displ = calloc(1, sizeof(struct Displ));

	struct State *s = calloc(1, sizeof(struct State));

	s->mode = calloc(1, sizeof(struct Mode));
	for (int i = 0; i < 10; i++) {
		struct Head *head = calloc(1, sizeof(struct Head));
		head->desired.enabled = true;
		head->desired.mode = s->mode;
		slist_append(&s->heads, head);
	}

	*state = s;
	return 0;
}

static int after_each(void **state) {
	slist_free(&g_heads);

	free(g_displ);

	cfg_destroy();

	struct State *s = *state;

	slist_free_vals(&s->heads, NULL);
	free(s->mode);

	free(s);
	return 0;
}


static void order_heads__exact_partial_regex(void **state) {
	struct SList *order_name_desc = NULL;
	struct SList *heads = NULL;
	struct SList *expected = NULL;

	// ORDER
	slist_append(&order_name_desc, strdup("exact0"));
	slist_append(&order_name_desc, strdup("exact1"));
	slist_append(&order_name_desc, strdup("!.*regex.*"));
	slist_append(&order_name_desc, strdup("exact1")); // should not repeat
	slist_append(&order_name_desc, strdup("partial"));

	// heads
	struct Head not_specified_1 = { .description = "not specified 1", };
	struct Head exact0_partial =  { .description = "not an exact0 exact match" };
	struct Head partial =         { .description = "a partial match" };
	struct Head regex_match_1 =   { .description = "a regex match" };
	struct Head exact1 =          { .description = "exact1" };
	struct Head exact0 =          { .description = "exact0" };
	struct Head regex_match_2 =   { .description = "another regex match" };
	struct Head not_specified_2 = { .description = "not specified 2" };
	slist_append(&heads, &not_specified_1);
	slist_append(&heads, &exact0_partial);
	slist_append(&heads, &partial);
	slist_append(&heads, &regex_match_1);
	slist_append(&heads, &exact1);
	slist_append(&heads, &exact0);
	slist_append(&heads, &regex_match_2);
	slist_append(&heads, &not_specified_2);

	// expected
	slist_append(&expected, &exact0);
	slist_append(&expected, &exact0_partial);
	slist_append(&expected, &exact1);
	slist_append(&expected, &regex_match_1);
	slist_append(&expected, &regex_match_2);
	slist_append(&expected, &partial);
	slist_append(&expected, &not_specified_1);
	slist_append(&expected, &not_specified_2);

	struct SList *heads_ordered = order_heads(order_name_desc, heads);

	assert_heads_order(heads_ordered, expected);

	slist_free_vals(&order_name_desc, NULL);
	slist_free(&heads);
	slist_free(&expected);
	slist_free(&heads_ordered);

	assert_logs_empty();
}

static void order_heads__exact_regex_catchall(void **state) {
	struct SList *order_name_desc = NULL;
	struct SList *heads = NULL;
	struct SList *expected = NULL;

	// ORDER
	slist_append(&order_name_desc, strdup("exact0"));
	slist_append(&order_name_desc, strdup("!.*regex.*"));
	slist_append(&order_name_desc, strdup("!.*$"));
	slist_append(&order_name_desc, strdup("exact9"));

	// heads
	struct Head exact9 =          { .description = "exact9" };
	struct Head not_specified_1 = { .description = "not specified 1", };
	struct Head regex_match_1 =   { .description = "a regex match" };
	struct Head exact0 =          { .description = "exact0" };
	struct Head regex_match_2 =   { .description = "another regex match" };
	struct Head not_specified_2 = { .description = "not specified 2" };
	slist_append(&heads, &not_specified_1);
	slist_append(&heads, &regex_match_1);
	slist_append(&heads, &exact0);
	slist_append(&heads, &regex_match_2);
	slist_append(&heads, &not_specified_2);
	slist_append(&heads, &exact9);

	// expected
	slist_append(&expected, &exact0);
	slist_append(&expected, &regex_match_1);
	slist_append(&expected, &regex_match_2);
	slist_append(&expected, &not_specified_1);
	slist_append(&expected, &not_specified_2);
	slist_append(&expected, &exact9);

	struct SList *heads_ordered = order_heads(order_name_desc, heads);

	assert_heads_order(heads_ordered, expected);

	slist_free_vals(&order_name_desc, NULL);
	slist_free(&heads);
	slist_free(&expected);
	slist_free(&heads_ordered);

	assert_logs_empty();
}

static void order_heads__no_order(void **state) {
	struct SList *heads = NULL;
	struct Head head = { .name = "head", };

	slist_append(&heads, &head);

	// null/empty order
	struct SList *heads_ordered = order_heads(NULL, heads);
	assert_heads_order(heads_ordered, heads);

	slist_free(&heads_ordered);
	slist_free(&heads);

	assert_logs_empty();
}

static void position_heads__col_left(void **state) {
	struct State *s = *state;
	struct Head *head;

	g_cfg->arrange = COL;
	g_cfg->align = LEFT;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 3;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	position_heads(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 0, 0);
	head = slist_at(s->heads, 1); assert_head_position(head, 0, 2);
	head = slist_at(s->heads, 2); assert_head_position(head, 0, 5);

	assert_logs_empty();
}

static void position_heads__col_mid(void **state) {
	struct State *s = *state;
	struct Head *head;

	g_cfg->arrange = COL;
	g_cfg->align = MIDDLE;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 3;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	position_heads(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 2, 0);
	head = slist_at(s->heads, 1); assert_head_position(head, 0, 2);
	head = slist_at(s->heads, 2); assert_head_position(head, 3, 5);

	assert_logs_empty();
}

static void position_heads__col_right(void **state) {
	struct State *s = *state;
	struct Head *head;

	g_cfg->arrange = COL;
	g_cfg->align = RIGHT;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 3;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	position_heads(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 3, 0);
	head = slist_at(s->heads, 1); assert_head_position(head, 0, 2);
	head = slist_at(s->heads, 2); assert_head_position(head, 5, 5);

	assert_logs_empty();
}

static void position_heads__row_top(void **state) {
	struct State *s = *state;
	struct Head *head;

	g_cfg->arrange = ROW;
	g_cfg->align = TOP;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 5;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	position_heads(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 0, 0);
	head = slist_at(s->heads, 1); assert_head_position(head, 4, 0);
	head = slist_at(s->heads, 2); assert_head_position(head, 11, 0);

	assert_logs_empty();
}

static void position_heads__row_mid(void **state) {
	struct State *s = *state;
	struct Head *head;

	g_cfg->arrange = ROW;
	g_cfg->align = MIDDLE;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 5;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	position_heads(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 0, 2);
	head = slist_at(s->heads, 1); assert_head_position(head, 4, 0);
	head = slist_at(s->heads, 2); assert_head_position(head, 11, 2);

	assert_logs_empty();
}

static void position_heads__row_bottom(void **state) {
	struct State *s = *state;
	struct Head *head;

	g_cfg->arrange = ROW;
	g_cfg->align = BOTTOM;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 5;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	position_heads(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 0, 3);
	head = slist_at(s->heads, 1); assert_head_position(head, 4, 0);
	head = slist_at(s->heads, 2); assert_head_position(head, 11, 4);

	assert_logs_empty();
}

static void desire_enabled__lid_closed_many(void **state) {
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = true,
	};
	slist_append(&g_heads, &head0);
	struct Head head1 = {
		.name = "head1",
		.desired.enabled = true,
	};
	slist_append(&g_heads, &head1);

	expect_str(__wrap_lid_is_closed, name, "head0");
	will_return_int(__wrap_lid_is_closed, true);

	desire_enabled(&head0);

	assert_false(head0.desired.enabled);

	assert_logs_empty();
}

static void desire_enabled__lid_closed_one(void **state) {
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = true,
	};
	slist_append(&g_heads, &head0);

	expect_str(__wrap_lid_is_closed, name, "head0");
	will_return_int(__wrap_lid_is_closed, true);

	desire_enabled(&head0);

	assert_true(head0.desired.enabled);

	assert_logs_empty();
}

static void desire_enabled__lid_closed_one_disabled(void **state) {
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = true,
	};
	slist_append(&g_heads, &head0);

	pset_add(g_cfg->disableds, disabled_init_always("![hH]ead[0-9]"));

	expect_str(__wrap_lid_is_closed, name, "head0");
	will_return_int(__wrap_lid_is_closed, true);

	desire_enabled(&head0);

	assert_false(head0.desired.enabled);

	assert_logs_empty();
}

static void desire_enabled__override(void **state) {
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = false,
		.overrided_enabled = OverrideTrue,
	};
	slist_append(&g_heads, &head0);

	pset_add(g_cfg->disableds, disabled_init_always("![hH]ead[0-9]"));

	expect_str(__wrap_lid_is_closed, name, "head0");
	will_return_int(__wrap_lid_is_closed, false);

	desire_enabled(&head0);

	assert_true(head0.desired.enabled);
	assert_true(head0.overrided_enabled == OverrideTrue);

	assert_logs_empty();
}

static void desire_enabled__override_reset(void **state) {
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = true,
		.overrided_enabled = OverrideFalse,
	};
	slist_append(&g_heads, &head0);

	pset_add(g_cfg->disableds, disabled_init_always("![hH]ead[0-9]"));

	expect_str(__wrap_lid_is_closed, name, "head0");
	will_return_int(__wrap_lid_is_closed, false);

	desire_enabled(&head0);

	assert_false(head0.desired.enabled);
	assert_true(head0.overrided_enabled == NoOverride);

	assert_logs_empty();
}

static void desire_mode__disabled(void **state) {
	struct Mode mode0 = { 0 };
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = false,
		.desired.mode = &mode0,
	};

	desire_mode(&head0);

	assert_ptr_equal(head0.desired.mode, &mode0);
	assert_false(head0.desired.enabled);
	assert_false(head0.warned_no_mode);

	assert_logs_empty();
}

static void desire_mode__no_mode(void **state) {
	struct Mode mode0 = { 0 };
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = true,
		.desired.mode = &mode0,
	};

	expect_ptr(__wrap_head_find_mode, head, &head0);
	will_return_ptr_type(__wrap_head_find_mode, NULL, struct Mode*);

	desire_mode(&head0);

	assert_ptr_equal(head0.desired.mode, &mode0);
	assert_false(head0.desired.enabled);
	assert_true(head0.warned_no_mode);

	assert_logs_empty();
}

static void desire_mode__no_mode_warned(void **state) {
	struct Mode mode0 = { 0 };
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = true,
		.desired.mode = &mode0,
		.warned_no_mode = true,
	};

	expect_ptr(__wrap_head_find_mode, head, &head0);
	will_return_ptr_type(__wrap_head_find_mode, NULL, struct Mode*);

	desire_mode(&head0);

	assert_ptr_equal(head0.desired.mode, &mode0);
	assert_false(head0.desired.enabled);
	assert_true(head0.warned_no_mode);

	assert_logs_empty();
}

static void desire_mode__ok(void **state) {
	struct Mode mode0 = { 0 };
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = true,
		.desired.mode = &mode0,
	};
	struct Mode mode1 = { 0 };

	expect_ptr(__wrap_head_find_mode, head, &head0);
	will_return_ptr_type(__wrap_head_find_mode, &mode1, struct Mode*);

	desire_mode(&head0);

	assert_ptr_equal(head0.desired.mode, &mode1);
	assert_true(head0.desired.enabled);
	assert_false(head0.warned_no_mode);

	assert_logs_empty();
}

static void desire_scale__disabled(void **state) {
	struct Head head0 = {
		.desired.enabled = false,
	};

	desire_scale(&head0);

	assert_logs_empty();
}

static void desire_scale__no_scaling(void **state) {
	struct Head head0 = {
		.desired.enabled = true,
	};
	g_cfg->scaling = OFF;
	g_cfg->auto_scale = ON;

	desire_scale(&head0);

	assert_wl_fixed_t_equal_double(head0.desired.scale, 1);

	assert_logs_empty();
}

static void desire_scale__no_auto(void **state) {
	struct Head head0 = {
		.desired.enabled = true,
	};
	g_cfg->scaling = ON;
	g_cfg->auto_scale = OFF;

	desire_scale(&head0);

	assert_wl_fixed_t_equal_double(head0.desired.scale, 1);

	assert_logs_empty();
}

static void desire_scale__auto(void **state) {
	struct Head head0 = {
		.desired.enabled = true,
	};
	g_cfg->scaling = ON;
	g_cfg->auto_scale = ON;

	expect_ptr(__wrap_head_auto_scale, head, &head0);
	will_return_int(__wrap_head_auto_scale, wl_fixed_from_double(2.5));

	desire_scale(&head0);

	assert_wl_fixed_t_equal_double(head0.desired.scale, 2.5);

	assert_logs_empty();
}

static void desire_scale__user(void **state) {
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = true,
	};
	g_cfg->scaling = ON;
	g_cfg->auto_scale = ON;

	smap_put(g_cfg->user_scales, "![Hh]ea.*", user_scale_init("![Hh]ea.*", 3.5));
	smap_put(g_cfg->user_scales, "head1", user_scale_init("head1", 7.5));

	desire_scale(&head0);

	assert_wl_fixed_t_equal_double(head0.desired.scale, 3.5);

	assert_logs_empty();
}

static void desire_transform__disabled(void **state) {
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = false,
		.desired.transform = WL_OUTPUT_TRANSFORM_90,
	};
	slist_append(&g_cfg->user_transforms, cfg_user_transform_init("head0", WL_OUTPUT_TRANSFORM_180));

	desire_transform(&head0);

	assert_int_equal(head0.desired.transform, WL_OUTPUT_TRANSFORM_90);

	assert_logs_empty();
}

static void desire_transform__no_transform(void **state) {
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = true,
		.desired.transform = WL_OUTPUT_TRANSFORM_90,
	};

	desire_transform(&head0);

	assert_int_equal(head0.desired.transform, WL_OUTPUT_TRANSFORM_NORMAL);

	assert_logs_empty();
}

static void desire_transform__user(void **state) {
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = true,
		.desired.transform = WL_OUTPUT_TRANSFORM_90,
	};
	slist_append(&g_cfg->user_transforms, cfg_user_transform_init("head9", WL_OUTPUT_TRANSFORM_270));
	slist_append(&g_cfg->user_transforms, cfg_user_transform_init("head0", WL_OUTPUT_TRANSFORM_180));

	desire_transform(&head0);

	assert_int_equal(head0.desired.transform, WL_OUTPUT_TRANSFORM_180);

	assert_logs_empty();
}

static void desire_adaptive_sync__head_disabled(void **state) {
	struct Head head0 = {
		.desired.enabled = false,
		.desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED,
	};

	desire_adaptive_sync(&head0);

	assert_int_equal(head0.desired.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED);

	assert_logs_empty();
}

static void desire_adaptive_sync__failed(void **state) {
	struct Head head0 = {
		.desired.enabled = true,
		.desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED,
		.adaptive_sync_failed = true,
	};

	desire_adaptive_sync(&head0);

	assert_int_equal(head0.desired.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED);

	assert_logs_empty();
}

static void desire_adaptive_sync__disabled(void **state) {
	struct Head head0 = {
		.name = "some head",
		.desired.enabled = true,
		.desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED,
	};

	sset_add(g_cfg->adaptive_sync_off, "!.*hea");

	desire_adaptive_sync(&head0);

	assert_int_equal(head0.desired.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED);

	assert_logs_empty();
}

static void desire_adaptive_sync__enabled(void **state) {
	struct Head head0 = {
		.desired.enabled = true,
		.desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED,
	};

	desire_adaptive_sync(&head0);

	assert_int_equal(head0.desired.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED);

	assert_logs_empty();
}

static void desire_reapply__required(void **state) {
	struct Head head0 = {
		.desired.enabled = true,
		.reapply_required = true,
	};

	desire_reapply(&head0);

	assert_false(head0.desired.enabled);

	assert_logs_empty();
}

static void desire_reapply__not_required(void **state) {
	struct Head head0 = {
		.desired.enabled = true,
		.reapply_required = false,
	};

	desire_reapply(&head0);

	assert_true(head0.desired.enabled);

	assert_logs_empty();
}

static void handle_success__head_changing_adaptive_sync(void **state) {
	struct Head head = {
		.desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED,
		.current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED,
		.adaptive_sync_failed = false,
	};
	g_displ->delta.element = VRR_OFF;
	g_displ->delta.head = &head;

	expect_int_value(__wrap_call_back, t, INFO);
	expect_str(__wrap_call_back, msg1, "Changes successful");
	expect_str(__wrap_call_back, msg2, NULL);

	handle_success();

	assert_log(INFO, "\nChanges successful\n");
	assert_logs_empty();

	assert_false(head.adaptive_sync_failed);
}

static void handle_success__head_changing_adaptive_sync_fail(void **state) {
	struct Head head = {
		.name = "head",
		.model = NULL, // fall back to placeholder
		.desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED,
		.current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED,
	};
	g_displ->delta.element = VRR_OFF;
	g_displ->delta.head = &head;

	expect_int_value(__wrap_print_adaptive_sync_fail, t, WARNING);
	expect_ptr(__wrap_print_adaptive_sync_fail, head, &head);

	expect_int_value(__wrap_call_back_adaptive_sync_fail, t, WARNING);
	expect_ptr(__wrap_call_back_adaptive_sync_fail, head, &head);

	handle_success();

	assert_true(head.adaptive_sync_failed);

	assert_logs_empty();
}

static void handle_success__head_changing_mode(void **state) {
	struct Mode mode = { 0 };
	struct Head head = {
		.desired.mode = &mode,
	};
	g_displ->delta.element = MODE;
	g_displ->delta.head = &head;

	expect_int_value(__wrap_call_back, t, INFO);
	expect_str(__wrap_call_back, msg1, "Changes successful");
	expect_str(__wrap_call_back, msg2, NULL);

	handle_success();

	assert_log(INFO, "\nChanges successful\n");
	assert_logs_empty();

	assert_ptr_equal(head.current.mode, &mode);
}

static void handle_success__ok(void **state) {
	g_displ->delta.human = strdup("human");

	expect_int_value(__wrap_call_back, t, INFO);
	expect_str(__wrap_call_back, msg1, "human");
	expect_str(__wrap_call_back, msg2, NULL);

	handle_success();

	assert_log(INFO, "\nChanges successful\n");
	assert_logs_empty();
}

static void handle_failure__mode(void **state) {
	struct Mode mode_cur = { 0 };
	struct Mode mode_des = { 0 };
	struct Head head = {
		.name = "nam",
		.current.mode = &mode_cur,
		.desired.mode = &mode_des,
	};
	g_displ->delta.element = MODE;
	g_displ->delta.head = &head;

	expect_int_value(__wrap_print_mode_fail, t, ERROR);
	expect_ptr(__wrap_print_mode_fail, head, &head);
	expect_ptr(__wrap_print_mode_fail, mode, &mode_des);

	expect_int_value(__wrap_call_back_mode_fail, t, ERROR);
	expect_ptr(__wrap_call_back_mode_fail, head, &head);
	expect_ptr(__wrap_call_back_mode_fail, mode, &mode_des);

	handle_failure();

	assert_nul(head.current.mode);
	assert_ptr_equal(head.desired.mode, &mode_des);

	assert_ptr_equal(slist_find_equal_val(head.modes_failed, NULL, &mode_des), &mode_des);

	slist_free(&head.modes_failed);

	assert_logs_empty();
}

static void handle_failure__adaptive_sync(void **state) {
	struct Head head = {
		.name = "nam",
		.model = "mod",
		.current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED,
		.desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED,
	};
	g_displ->delta.element = VRR_OFF;
	g_displ->delta.head = &head;

	expect_int_value(__wrap_print_adaptive_sync_fail, t, WARNING);
	expect_ptr(__wrap_print_adaptive_sync_fail, head, &head);

	expect_int_value(__wrap_call_back_adaptive_sync_fail, t, WARNING);
	expect_ptr(__wrap_call_back_adaptive_sync_fail, head, &head);

	handle_failure();

	assert_true(head.adaptive_sync_failed);

	assert_logs_empty();
}

static void handle_failure__unspecified(void **state) {
	g_displ->delta.human = strdup("human");

	expect_int_value(__wrap_call_back, t, FATAL);
	expect_str(__wrap_call_back, msg1, "human");
	expect_str(__wrap_call_back, msg2, "\nChanges failed, exiting");

	expect_int_value(__wrap_wd_exit_message, __status, EXIT_FAILURE);

	handle_failure();

	assert_log(FATAL, "\nChanges failed, exiting\n");
	assert_logs_empty();
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(order_heads__exact_partial_regex),
		TEST_BA(order_heads__exact_regex_catchall),
		TEST_BA(order_heads__no_order),

		TEST_BA(position_heads__col_left),
		TEST_BA(position_heads__col_mid),
		TEST_BA(position_heads__col_right),
		TEST_BA(position_heads__row_top),
		TEST_BA(position_heads__row_mid),
		TEST_BA(position_heads__row_bottom),

		TEST_BA(desire_enabled__lid_closed_many),
		TEST_BA(desire_enabled__lid_closed_one_disabled),
		TEST_BA(desire_enabled__lid_closed_one),
		TEST_BA(desire_enabled__override),
		TEST_BA(desire_enabled__override_reset),

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

		TEST_BA(desire_reapply__required),
		TEST_BA(desire_reapply__not_required),

		TEST_BA(handle_success__head_changing_adaptive_sync),
		TEST_BA(handle_success__head_changing_adaptive_sync_fail),
		TEST_BA(handle_success__head_changing_mode),
		TEST_BA(handle_success__ok),

		TEST_BA(handle_failure__mode),
		TEST_BA(handle_failure__adaptive_sync),
		TEST_BA(handle_failure__unspecified),
	};

	return RUN(tests);
}

