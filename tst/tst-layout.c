#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "cfg.h"
#include "displ.h"
#include "global.h"
#include "head.h"
#include "log.h"
#include "mode.h"
#include "slist.h"
#include "wlr-output-management-unstable-v1.h"

#include "layout.h"

struct Mode *__wrap_head_find_mode(struct Head *head) {
	check_expected(head);
	return mock_type(struct Mode*);
}

wl_fixed_t __wrap_head_auto_scale(struct Head *head) {
	check_expected(head);
	return mock_type(wl_fixed_t);
}


struct State {
	struct Mode *mode;
	struct SList *heads;
};


int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	cfg = cfg_default();

	// only set this when we specifically want to test it
	free(cfg->callback_cmd);
	cfg->callback_cmd = NULL;

	displ = calloc(1, sizeof(struct Displ));

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

int after_each(void **state) {
	assert_logs_empty();

	slist_free(&heads);

	assert_nul(displ->delta.head);
	assert_int_equal(displ->delta.element, 0);
	assert_nul(displ->delta.human);

	free(displ);

	cfg_destroy();

	struct State *s = *state;

	slist_free_vals(&s->heads, NULL);
	free(s->mode);

	free(s);
	return 0;
}


void order_heads__exact_partial_regex(void **state) {
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
}

void order_heads__exact_regex_catchall(void **state) {
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
}

void order_heads__no_order(void **state) {
	struct SList *heads = NULL;
	struct Head head = { .name = "head", };

	slist_append(&heads, &head);

	// null/empty order
	struct SList *heads_ordered = order_heads(NULL, heads);
	assert_heads_order(heads_ordered, heads);

	slist_free(&heads_ordered);
	slist_free(&heads);
}

void position_heads__col_left(void **state) {
	struct State *s = *state;
	struct Head *head;

	cfg->arrange = COL;
	cfg->align = LEFT;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 3;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	position_heads(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 0, 0);
	head = slist_at(s->heads, 1); assert_head_position(head, 0, 2);
	head = slist_at(s->heads, 2); assert_head_position(head, 0, 5);
}

void position_heads__col_mid(void **state) {
	struct State *s = *state;
	struct Head *head;

	cfg->arrange = COL;
	cfg->align = MIDDLE;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 3;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	position_heads(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 2, 0);
	head = slist_at(s->heads, 1); assert_head_position(head, 0, 2);
	head = slist_at(s->heads, 2); assert_head_position(head, 3, 5);
}

void position_heads__col_right(void **state) {
	struct State *s = *state;
	struct Head *head;

	cfg->arrange = COL;
	cfg->align = RIGHT;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 3;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	position_heads(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 3, 0);
	head = slist_at(s->heads, 1); assert_head_position(head, 0, 2);
	head = slist_at(s->heads, 2); assert_head_position(head, 5, 5);
}

void position_heads__row_top(void **state) {
	struct State *s = *state;
	struct Head *head;

	cfg->arrange = ROW;
	cfg->align = TOP;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 5;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	position_heads(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 0, 0);
	head = slist_at(s->heads, 1); assert_head_position(head, 4, 0);
	head = slist_at(s->heads, 2); assert_head_position(head, 11, 0);
}

void position_heads__row_mid(void **state) {
	struct State *s = *state;
	struct Head *head;

	cfg->arrange = ROW;
	cfg->align = MIDDLE;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 5;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	position_heads(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 0, 2);
	head = slist_at(s->heads, 1); assert_head_position(head, 4, 0);
	head = slist_at(s->heads, 2); assert_head_position(head, 11, 2);
}

void position_heads__row_bottom(void **state) {
	struct State *s = *state;
	struct Head *head;

	cfg->arrange = ROW;
	cfg->align = BOTTOM;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 5;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	position_heads(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 0, 3);
	head = slist_at(s->heads, 1); assert_head_position(head, 4, 0);
	head = slist_at(s->heads, 2); assert_head_position(head, 11, 4);
}

void desire_enabled__lid_closed_many(void **state) {
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = true,
	};
	slist_append(&heads, &head0);
	struct Head head1 = {
		.name = "head1",
		.desired.enabled = true,
	};
	slist_append(&heads, &head1);

	expect_string(__wrap_lid_is_closed, name, "head0");
	will_return(__wrap_lid_is_closed, true);

	desire_enabled(&head0);

	assert_false(head0.desired.enabled);
}

void desire_enabled__lid_closed_one(void **state) {
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = true,
	};
	slist_append(&heads, &head0);

	expect_string(__wrap_lid_is_closed, name, "head0");
	will_return(__wrap_lid_is_closed, true);

	desire_enabled(&head0);

	assert_true(head0.desired.enabled);
}

void desire_enabled__lid_closed_one_disabled(void **state) {
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = true,
	};
	slist_append(&heads, &head0);

	slist_append(&cfg->disabled, cfg_disabled_always("![hH]ead[0-9]"));

	expect_string(__wrap_lid_is_closed, name, "head0");
	will_return(__wrap_lid_is_closed, true);

	desire_enabled(&head0);

	assert_false(head0.desired.enabled);
}

void desire_enabled__override(void **state) {
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = false,
		.overrided_enabled = OverrideTrue,
	};
	slist_append(&heads, &head0);

	slist_append(&cfg->disabled, cfg_disabled_always("![hH]ead[0-9]"));

	expect_string(__wrap_lid_is_closed, name, "head0");
	will_return(__wrap_lid_is_closed, false);

	desire_enabled(&head0);

	assert_true(head0.desired.enabled);
	assert_true(head0.overrided_enabled == OverrideTrue);
}

void desire_enabled__override_reset(void **state) {
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = true,
		.overrided_enabled = OverrideFalse,
	};
	slist_append(&heads, &head0);

	slist_append(&cfg->disabled, cfg_disabled_always("![hH]ead[0-9]"));

	expect_string(__wrap_lid_is_closed, name, "head0");
	will_return(__wrap_lid_is_closed, false);

	desire_enabled(&head0);

	assert_false(head0.desired.enabled);
	assert_true(head0.overrided_enabled == NoOverride);
}

void desire_mode__disabled(void **state) {
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
}

void desire_mode__no_mode(void **state) {
	struct Mode mode0 = { 0 };
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = true,
		.desired.mode = &mode0,
	};

	expect_value(__wrap_head_find_mode, head, &head0);
	will_return(__wrap_head_find_mode, NULL);

	desire_mode(&head0);

	assert_ptr_equal(head0.desired.mode, &mode0);
	assert_false(head0.desired.enabled);
	assert_true(head0.warned_no_mode);
}

void desire_mode__no_mode_warned(void **state) {
	struct Mode mode0 = { 0 };
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = true,
		.desired.mode = &mode0,
		.warned_no_mode = true,
	};

	expect_value(__wrap_head_find_mode, head, &head0);
	will_return(__wrap_head_find_mode, NULL);

	desire_mode(&head0);

	assert_ptr_equal(head0.desired.mode, &mode0);
	assert_false(head0.desired.enabled);
	assert_true(head0.warned_no_mode);
}

void desire_mode__ok(void **state) {
	struct Mode mode0 = { 0 };
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = true,
		.desired.mode = &mode0,
	};
	struct Mode mode1 = { 0 };

	expect_value(__wrap_head_find_mode, head, &head0);
	will_return(__wrap_head_find_mode, &mode1);

	desire_mode(&head0);

	assert_ptr_equal(head0.desired.mode, &mode1);
	assert_true(head0.desired.enabled);
	assert_false(head0.warned_no_mode);
}

void desire_scale__disabled(void **state) {
	struct Head head0 = {
		.desired.enabled = false,
	};

	desire_scale(&head0);
}

void desire_scale__no_scaling(void **state) {
	struct Head head0 = {
		.desired.enabled = true,
	};
	cfg->scaling = OFF;
	cfg->auto_scale = ON;

	desire_scale(&head0);

	assert_wl_fixed_t_equal_double(head0.desired.scale, 1);
}

void desire_scale__no_auto(void **state) {
	struct Head head0 = {
		.desired.enabled = true,
	};
	cfg->scaling = ON;
	cfg->auto_scale = OFF;

	desire_scale(&head0);

	assert_wl_fixed_t_equal_double(head0.desired.scale, 1);
}

void desire_scale__auto(void **state) {
	struct Head head0 = {
		.desired.enabled = true,
	};
	cfg->scaling = ON;
	cfg->auto_scale = ON;

	expect_value(__wrap_head_auto_scale, head, &head0);
	will_return(__wrap_head_auto_scale, wl_fixed_from_double(2.5));

	desire_scale(&head0);

	assert_wl_fixed_t_equal_double(head0.desired.scale, 2.5);
}

void desire_scale__user(void **state) {
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = true,
	};
	cfg->scaling = ON;
	cfg->auto_scale = ON;

	slist_append(&cfg->user_scales, cfg_user_scale_init("![Hh]ea.*", 3.5));
	slist_append(&cfg->user_scales, cfg_user_scale_init("head1", 7.5));

	desire_scale(&head0);

	assert_wl_fixed_t_equal_double(head0.desired.scale, 3.5);
}

void desire_transform__disabled(void **state) {
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = false,
		.desired.transform = WL_OUTPUT_TRANSFORM_90,
	};
	slist_append(&cfg->user_transforms, cfg_user_transform_init("head0", WL_OUTPUT_TRANSFORM_180));

	desire_transform(&head0);

	assert_int_equal(head0.desired.transform, WL_OUTPUT_TRANSFORM_90);
}

void desire_transform__no_transform(void **state) {
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = true,
		.desired.transform = WL_OUTPUT_TRANSFORM_90,
	};

	desire_transform(&head0);

	assert_int_equal(head0.desired.transform, WL_OUTPUT_TRANSFORM_NORMAL);
}

void desire_transform__user(void **state) {
	struct Head head0 = {
		.name = "head0",
		.desired.enabled = true,
		.desired.transform = WL_OUTPUT_TRANSFORM_90,
	};
	slist_append(&cfg->user_transforms, cfg_user_transform_init("head9", WL_OUTPUT_TRANSFORM_270));
	slist_append(&cfg->user_transforms, cfg_user_transform_init("head0", WL_OUTPUT_TRANSFORM_180));

	desire_transform(&head0);

	assert_int_equal(head0.desired.transform, WL_OUTPUT_TRANSFORM_180);
}

void desire_adaptive_sync__head_disabled(void **state) {
	struct Head head0 = {
		.desired.enabled = false,
		.desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED,
	};

	desire_adaptive_sync(&head0);

	assert_int_equal(head0.desired.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED);
}

void desire_adaptive_sync__failed(void **state) {
	struct Head head0 = {
		.desired.enabled = true,
		.desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED,
		.adaptive_sync_failed = true,
	};

	desire_adaptive_sync(&head0);

	assert_int_equal(head0.desired.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED);
}

void desire_adaptive_sync__disabled(void **state) {
	struct Head head0 = {
		.name = "some head",
		.desired.enabled = true,
		.desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED,
	};

	slist_append(&cfg->adaptive_sync_off_name_desc, strdup("!.*hea"));

	desire_adaptive_sync(&head0);

	assert_int_equal(head0.desired.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED);
}

void desire_adaptive_sync__enabled(void **state) {
	struct Head head0 = {
		.desired.enabled = true,
		.desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED,
	};

	desire_adaptive_sync(&head0);

	assert_int_equal(head0.desired.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED);
}

void handle_success__head_changing_adaptive_sync(void **state) {
	struct Head head = {
		.desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED,
		.current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED,
		.adaptive_sync_failed = false,
	};
	displ->delta.element = VRR_OFF;
	displ->delta.head = &head;

	expect_value(__wrap_call_back, t, INFO);
	expect_string(__wrap_call_back, msg1, "Changes successful");
	expect_value(__wrap_call_back, msg2, NULL);

	handle_success();

	assert_log(INFO, "\nChanges successful\n");

	assert_false(head.adaptive_sync_failed);
}

void handle_success__head_changing_adaptive_sync_fail(void **state) {
	struct Head head = {
		.name = "head",
		.model = NULL, // fall back to placeholder
		.desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED,
		.current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED,
	};
	displ->delta.element = VRR_OFF;
	displ->delta.head = &head;

	expect_value(__wrap_print_adaptive_sync_fail, t, WARNING);
	expect_value(__wrap_print_adaptive_sync_fail, head, &head);

	expect_value(__wrap_call_back_adaptive_sync_fail, t, WARNING);
	expect_string(__wrap_call_back_adaptive_sync_fail, head, &head);

	handle_success();

	assert_true(head.adaptive_sync_failed);
}

void handle_success__head_changing_mode(void **state) {
	struct Mode mode = { 0 };
	struct Head head = {
		.desired.mode = &mode,
	};
	displ->delta.element = MODE;
	displ->delta.head = &head;

	expect_value(__wrap_call_back, t, INFO);
	expect_string(__wrap_call_back, msg1, "Changes successful");
	expect_value(__wrap_call_back, msg2, NULL);

	handle_success();

	assert_log(INFO, "\nChanges successful\n");

	assert_ptr_equal(head.current.mode, &mode);
}

void handle_success__ok(void **state) {
	displ->delta.human = strdup("human");

	expect_value(__wrap_call_back, t, INFO);
	expect_string(__wrap_call_back, msg1, "human");
	expect_value(__wrap_call_back, msg2, NULL);

	handle_success();

	assert_log(INFO, "\nChanges successful\n");
}

void handle_failure__mode(void **state) {
	struct Mode mode_cur = { 0 };
	struct Mode mode_des = { 0 };
	struct Head head = {
		.name = "nam",
		.current.mode = &mode_cur,
		.desired.mode = &mode_des,
	};
	displ->delta.element = MODE;
	displ->delta.head = &head;

	expect_value(__wrap_print_mode_fail, t, ERROR);
	expect_value(__wrap_print_mode_fail, head, &head);
	expect_value(__wrap_print_mode_fail, mode, &mode_des);

	expect_value(__wrap_call_back_mode_fail, t, ERROR);
	expect_value(__wrap_call_back_mode_fail, head, &head);
	expect_value(__wrap_call_back_mode_fail, mode, &mode_des);

	handle_failure();

	assert_nul(head.current.mode);
	assert_ptr_equal(head.desired.mode, &mode_des);

	assert_ptr_equal(slist_find_equal_val(head.modes_failed, NULL, &mode_des), &mode_des);

	slist_free(&head.modes_failed);
}

void handle_failure__adaptive_sync(void **state) {
	struct Head head = {
		.name = "nam",
		.model = "mod",
		.current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED,
		.desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED,
	};
	displ->delta.element = VRR_OFF;
	displ->delta.head = &head;

	expect_value(__wrap_print_adaptive_sync_fail, t, WARNING);
	expect_value(__wrap_print_adaptive_sync_fail, head, &head);

	expect_value(__wrap_call_back_adaptive_sync_fail, t, WARNING);
	expect_string(__wrap_call_back_adaptive_sync_fail, head, &head);

	handle_failure();

	assert_true(head.adaptive_sync_failed);
}

void handle_failure__unspecified(void **state) {
	displ->delta.human = strdup("human");

	expect_value(__wrap_call_back, t, FATAL);
	expect_string(__wrap_call_back, msg1, "human");
	expect_string(__wrap_call_back, msg2, "\nChanges failed, exiting");

	expect_value(__wrap_wd_exit_message, __status, EXIT_FAILURE);

	handle_failure();

	assert_log(FATAL, "\nChanges failed, exiting\n");
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(order_heads__exact_partial_regex),
		TEST(order_heads__exact_regex_catchall),
		TEST(order_heads__no_order),

		TEST(position_heads__col_left),
		TEST(position_heads__col_mid),
		TEST(position_heads__col_right),
		TEST(position_heads__row_top),
		TEST(position_heads__row_mid),
		TEST(position_heads__row_bottom),

		TEST(desire_enabled__lid_closed_many),
		TEST(desire_enabled__lid_closed_one_disabled),
		TEST(desire_enabled__lid_closed_one),
		TEST(desire_enabled__override),
		TEST(desire_enabled__override_reset),

		TEST(desire_mode__disabled),
		TEST(desire_mode__no_mode),
		TEST(desire_mode__no_mode_warned),
		TEST(desire_mode__ok),

		TEST(desire_scale__disabled),
		TEST(desire_scale__no_scaling),
		TEST(desire_scale__no_auto),
		TEST(desire_scale__auto),
		TEST(desire_scale__user),

		TEST(desire_transform__disabled),
		TEST(desire_transform__no_transform),
		TEST(desire_transform__user),

		TEST(desire_adaptive_sync__head_disabled),
		TEST(desire_adaptive_sync__failed),
		TEST(desire_adaptive_sync__disabled),
		TEST(desire_adaptive_sync__enabled),

		TEST(handle_success__head_changing_adaptive_sync),
		TEST(handle_success__head_changing_adaptive_sync_fail),
		TEST(handle_success__head_changing_mode),
		TEST(handle_success__ok),

		TEST(handle_failure__mode),
		TEST(handle_failure__adaptive_sync),
		TEST(handle_failure__unspecified),
	};

	return RUN(tests);
}

