#include "tst.h"
#include "asserts.h"
#include "util.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "global.h"

#include "cfg.h"
#include "conditions.h"
#include "log.h"
#include "slist.h"

struct State {
	struct Cfg *from;
	struct Cfg *to;
	struct Cfg *expected;
};

struct Disabled *cfg_disabled_conditionally(char *name_desc, char *plugged) {
	struct Disabled *disabled1 = calloc(1, sizeof(struct Disabled));
	disabled1->name_desc = strdup(name_desc);
	struct Condition *cond1 = calloc(1, sizeof(struct Condition));
	slist_append(&cond1->plugged, strdup(plugged));
	slist_append(&disabled1->conditions, cond1);

	return disabled1;
}

int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	struct State *s = calloc(1, sizeof(struct State));

	slist_free_vals(&cfg_file_paths, NULL);

	cfg = cfg_default();

	s->from = cfg_default();
	s->to = cfg_default();
	s->expected = cfg_default();

	*state = s;
	return 0;
}

int after_each(void **state) {
	struct State *s = *state;

	slist_free_vals(&cfg_file_paths, NULL);

	cfg_destroy();

	assert_logs_empty();

	cfg_free(s->from);
	cfg_free(s->to);
	cfg_free(s->expected);

	free(s);
	return 0;
}


void merge_set__arrange(void **state) {
	struct State *s = *state;

	s->from->arrange = COL;
	s->expected->arrange = COL;

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__align(void **state) {
	struct State *s = *state;

	s->from->align = MIDDLE;
	s->expected->align = MIDDLE;

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__order(void **state) {
	struct State *s = *state;

	slist_append(&s->to->order_name_desc, strdup("A"));
	slist_append(&s->from->order_name_desc, strdup("X"));

	slist_append(&s->expected->order_name_desc, strdup("X"));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__auto_scale(void **state) {
	struct State *s = *state;

	s->from->auto_scale = OFF;
	s->expected->auto_scale = OFF;

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__user_scale(void **state) {
	struct State *s = *state;

	slist_append(&s->to->user_scales, cfg_user_scale_init("to", 1));
	slist_append(&s->to->user_scales, cfg_user_scale_init("both", 2));

	slist_append(&s->from->user_scales, cfg_user_scale_init("from", 3));
	slist_append(&s->from->user_scales, cfg_user_scale_init("both", 4));

	slist_append(&s->expected->user_scales, cfg_user_scale_init("to", 1));
	slist_append(&s->expected->user_scales, cfg_user_scale_init("both", 4));
	slist_append(&s->expected->user_scales, cfg_user_scale_init("from", 3));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__user_transform(void **state) {
	struct State *s = *state;

	slist_append(&s->to->user_transforms, cfg_user_transform_init("to", 1));
	slist_append(&s->to->user_transforms, cfg_user_transform_init("both", 2));

	slist_append(&s->from->user_transforms, cfg_user_transform_init("from", 3));
	slist_append(&s->from->user_transforms, cfg_user_transform_init("both", 4));

	slist_append(&s->expected->user_transforms, cfg_user_transform_init("to", 1));
	slist_append(&s->expected->user_transforms, cfg_user_transform_init("both", 4));
	slist_append(&s->expected->user_transforms, cfg_user_transform_init("from", 3));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__mode(void **state) {
	struct State *s = *state;

	slist_append(&s->to->user_modes, cfg_user_mode_init("to", false, 1, 2, 3, false));
	slist_append(&s->to->user_modes, cfg_user_mode_init("both", false, 4, 5, 6, false));

	slist_append(&s->from->user_modes, cfg_user_mode_init("from", false, 7, 8, 9, true));
	slist_append(&s->from->user_modes, cfg_user_mode_init("both", false, 10, 11, 12, true));

	slist_append(&s->expected->user_modes, cfg_user_mode_init("to", false, 1, 2, 3, false));
	slist_append(&s->expected->user_modes, cfg_user_mode_init("both", false, 10, 11, 12, true));
	slist_append(&s->expected->user_modes, cfg_user_mode_init("from", false, 7, 8, 9, true));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__adaptive_sync_off(void **state) {
	struct State *s = *state;

	slist_append(&s->to->adaptive_sync_off_name_desc, strdup("to"));
	slist_append(&s->to->adaptive_sync_off_name_desc, strdup("both"));

	slist_append(&s->from->adaptive_sync_off_name_desc, strdup("from"));
	slist_append(&s->from->adaptive_sync_off_name_desc, strdup("both"));

	slist_append(&s->expected->adaptive_sync_off_name_desc, strdup("to"));
	slist_append(&s->expected->adaptive_sync_off_name_desc, strdup("both"));
	slist_append(&s->expected->adaptive_sync_off_name_desc, strdup("from"));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__disabled(void **state) {
	struct State *s = *state;

	slist_append(&s->to->disabled, cfg_disabled_always("to"));
	slist_append(&s->to->disabled, cfg_disabled_always("both"));
	slist_append(&s->to->disabled, cfg_disabled_conditionally("to_cond", "keep"));
	slist_append(&s->to->disabled, cfg_disabled_conditionally("both_cond", "keep"));

	slist_append(&s->from->disabled, cfg_disabled_always("from"));
	slist_append(&s->from->disabled, cfg_disabled_always("both"));
	slist_append(&s->from->disabled, cfg_disabled_conditionally("from_cond", "keep"));
	slist_append(&s->from->disabled, cfg_disabled_conditionally("both_cond", "discard")); // merge_set will free this

	slist_append(&s->expected->disabled, cfg_disabled_always("to"));
	slist_append(&s->expected->disabled, cfg_disabled_always("both"));
	slist_append(&s->expected->disabled, cfg_disabled_conditionally("to_cond", "keep"));
	slist_append(&s->expected->disabled, cfg_disabled_conditionally("both_cond", "keep"));
	slist_append(&s->expected->disabled, cfg_disabled_always("from"));
	slist_append(&s->expected->disabled, cfg_disabled_conditionally("from_cond", "keep"));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__callback_cmd(void **state) {
	struct State *s = *state;

	free(s->to->callback_cmd);
	s->to->callback_cmd = strdup("to");

	free(s->from->callback_cmd);
	s->from->callback_cmd = strdup("from");

	free(s->expected->callback_cmd);
	s->expected->callback_cmd = strdup("from");

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_del__scale(void **state) {
	struct State *s = *state;

	slist_append(&s->to->user_scales, cfg_user_scale_init("1", 1));
	slist_append(&s->to->user_scales, cfg_user_scale_init("2", 2));

	slist_append(&s->from->user_scales, cfg_user_scale_init("2", 3));
	slist_append(&s->from->user_scales, cfg_user_scale_init("3", 4));

	slist_append(&s->expected->user_scales, cfg_user_scale_init("1", 1));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_del__mode(void **state) {
	struct State *s = *state;

	slist_append(&s->to->user_modes, cfg_user_mode_init("1", false, 1, 1, 1, false));
	slist_append(&s->to->user_modes, cfg_user_mode_init("2", false, 2, 2, 2, false));

	slist_append(&s->from->user_modes, cfg_user_mode_init("2", false, 2, 2, 2, false));
	slist_append(&s->from->user_modes, cfg_user_mode_init("3", false, 3, 3, 3, false));

	slist_append(&s->from->user_modes, cfg_user_mode_init("1", false, 1, 1, 1, false));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_del__transform(void **state) {
	struct State *s = *state;

	slist_append(&s->to->user_transforms, cfg_user_transform_init("to", 1));
	slist_append(&s->to->user_transforms, cfg_user_transform_init("both", 2));

	slist_append(&s->from->user_transforms, cfg_user_transform_init("from", 3));
	slist_append(&s->from->user_transforms, cfg_user_transform_init("both", 4));

	slist_append(&s->expected->user_transforms, cfg_user_transform_init("to", 1));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_del__adaptive_sync_off(void **state) {
	struct State *s = *state;

	slist_append(&s->to->adaptive_sync_off_name_desc, strdup("1"));
	slist_append(&s->to->adaptive_sync_off_name_desc, strdup("2"));

	slist_append(&s->from->adaptive_sync_off_name_desc, strdup("2"));
	slist_append(&s->from->adaptive_sync_off_name_desc, strdup("3"));

	slist_append(&s->expected->adaptive_sync_off_name_desc, strdup("1"));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_del__disabled(void **state) {
	struct State *s = *state;

	slist_append(&s->to->disabled, cfg_disabled_always("to"));
	slist_append(&s->to->disabled, cfg_disabled_always("both"));
	slist_append(&s->to->disabled, cfg_disabled_conditionally("cond1", "disp1"));
	slist_append(&s->to->disabled, cfg_disabled_conditionally("cond2", "disp2"));

	slist_append(&s->from->disabled, cfg_disabled_always("from"));
	slist_append(&s->from->disabled, cfg_disabled_always("both"));
	slist_append(&s->from->disabled, cfg_disabled_always("cond1"));
	slist_append(&s->from->disabled, cfg_disabled_conditionally("cond2", "disp2"));

	slist_append(&s->expected->disabled, cfg_disabled_always("to"));
	slist_append(&s->expected->disabled, cfg_disabled_conditionally("cond1", "disp1"));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_del__callback_cmd(void **state) {
	struct State *s = *state;

	free(s->to->callback_cmd);
	s->to->callback_cmd = strdup("to");

	free(s->from->callback_cmd);
	s->from->callback_cmd = strdup("");

	free(s->expected->callback_cmd);
	s->expected->callback_cmd = NULL;

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_toggle__scaling(void **state) {
	struct State *s = *state;

	s->to->scaling = ON;

	s->from->scaling = ON;
	s->from->auto_scale = OFF;

	s->expected->scaling = OFF;

	struct Cfg *merged = merge_toggle(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_toggle__auto_scale(void **state) {
	struct State *s = *state;

	s->to->auto_scale = OFF;

	s->from->scaling = OFF;
	s->from->auto_scale = ON;

	s->expected->auto_scale = ON;

	struct Cfg *merged = merge_toggle(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_toggle__adaptive_sync_off(void **state) {
	struct State *s = *state;

	s->from->auto_scale = false;
	s->from->scaling = false;

	slist_append(&s->to->adaptive_sync_off_name_desc, strdup("display1"));
	slist_append(&s->to->adaptive_sync_off_name_desc, strdup("display2"));

	slist_append(&s->from->adaptive_sync_off_name_desc, strdup("display2"));
	slist_append(&s->from->adaptive_sync_off_name_desc, strdup("display3"));

	slist_append(&s->expected->adaptive_sync_off_name_desc, strdup("display1"));
	slist_append(&s->expected->adaptive_sync_off_name_desc, strdup("display3"));

	struct Cfg *merged = merge_toggle(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_toggle__disabled(void **state) {
	struct State *s = *state;

	s->from->auto_scale = false;
	s->from->scaling = false;

	slist_append(&s->to->disabled, cfg_disabled_always("to"));
	slist_append(&s->to->disabled, cfg_disabled_always("both"));
	slist_append(&s->to->disabled, cfg_disabled_conditionally("cond_no_clobber", ""));
	slist_append(&s->to->disabled, cfg_disabled_conditionally("cond_no_toggle", ""));

	slist_append(&s->from->disabled, cfg_disabled_always("both"));
	slist_append(&s->from->disabled, cfg_disabled_always("from"));
	slist_append(&s->from->disabled, cfg_disabled_always("cond_no_clobber"));
	slist_append(&s->from->disabled, cfg_disabled_conditionally("cond_no_toggle", ""));

	slist_append(&s->expected->disabled, cfg_disabled_always("to"));
	slist_append(&s->expected->disabled, cfg_disabled_conditionally("cond_no_clobber", ""));
	slist_append(&s->expected->disabled, cfg_disabled_conditionally("cond_no_toggle", ""));
	slist_append(&s->expected->disabled, cfg_disabled_always("from"));

	struct Cfg *merged = merge_toggle(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void validate_fix__col(void **state) {
	struct State *s = *state;

	s->from->arrange = COL;
	s->from->align = TOP;

	s->expected->arrange = COL;
	s->expected->align = LEFT;

	validate_fix(s->from);

	assert_log(WARNING, "\nIgnoring invalid ALIGN TOP for COLUMN arrange. Valid values are LEFT, MIDDLE and RIGHT. Using default LEFT.\n");

	assert_cfg_equal(s->from, s->expected);
}

void validate_fix__row(void **state) {
	struct State *s = *state;

	s->from->arrange = ROW;
	s->from->align = RIGHT;

	s->expected->arrange = ROW;
	s->expected->align = TOP;

	validate_fix(s->from);

	assert_log(WARNING, "\nIgnoring invalid ALIGN RIGHT for ROW arrange. Valid values are TOP, MIDDLE and BOTTOM. Using default TOP.\n");

	assert_cfg_equal(s->from, s->expected);
}

void validate_fix__scale(void **state) {
	struct State *s = *state;

	slist_append(&s->from->user_scales, cfg_user_scale_init("ok", 1));

	slist_append(&s->from->user_scales, cfg_user_scale_init("neg", -1));

	slist_append(&s->from->user_scales, cfg_user_scale_init("zero", 0));

	validate_fix(s->from);

	char *expected_log = read_file("tst/cfg/validate-fix-scale.log");
	assert_log(WARNING, expected_log);

	slist_append(&s->expected->user_scales, cfg_user_scale_init("ok", 1));

	assert_cfg_equal(s->from, s->expected);

	free(expected_log);
}

void validate_fix__mode(void **state) {
	struct State *s = *state;

	slist_append(&s->from->user_modes, cfg_user_mode_init("ok", false, 1, 2, 3, false));
	slist_append(&s->from->user_modes, cfg_user_mode_init("max", true, -1, -1, -1, false));

	slist_append(&s->from->user_modes, cfg_user_mode_init("negative width", false, -99, 2, 3, false));

	slist_append(&s->from->user_modes, cfg_user_mode_init("negative height", false, 1, -99, 3, false));

	slist_append(&s->from->user_modes, cfg_user_mode_init("negative hz", false, 1, 2, -12340, false));

	slist_append(&s->from->user_modes, cfg_user_mode_init("missing width", false, -1, 2, 3, false));

	slist_append(&s->from->user_modes, cfg_user_mode_init("missing height", false, 1, -1, 3, false));

	validate_fix(s->from);

	char *expected_log = read_file("tst/cfg/validate-fix-mode.log");
	assert_log(WARNING, expected_log);

	slist_append(&s->expected->user_modes, cfg_user_mode_init("ok", false, 1, 2, 3, false));
	slist_append(&s->expected->user_modes, cfg_user_mode_init("max", true, -1, -1, -1, false));

	assert_cfg_equal(s->from, s->expected);

	free(expected_log);
}

void validate_fix__disabled(void **state) {
	struct State *s = *state;

	slist_append(&s->from->disabled, cfg_disabled_always("1"));
	slist_append(&s->from->disabled, cfg_disabled_always("2"));
	slist_append(&s->from->disabled, cfg_disabled_always("1"));

	slist_append(&s->expected->disabled, cfg_disabled_always("1"));
	slist_append(&s->expected->disabled, cfg_disabled_always("2"));

	validate_fix(s->from);

	assert_log(WARNING, "\nIgnoring duplicate DISABLED 1\n");

	assert_cfg_equal(s->from, s->expected);
}

void validate_warn__(void **state) {
	struct State *s = *state;

	slist_append(&s->expected->user_scales, cfg_user_scale_init("sss", 1));
	slist_append(&s->expected->user_scales, cfg_user_scale_init("ssssssss", 2));

	slist_append(&s->expected->user_modes, cfg_user_mode_init("mmm", false, 1, 1, 1, false));
	slist_append(&s->expected->user_modes, cfg_user_mode_init("mmmmmmmm", false, 1, 1, 1, false));

	slist_append(&s->expected->user_transforms, cfg_user_transform_init("ttt", WL_OUTPUT_TRANSFORM_180));
	slist_append(&s->expected->user_transforms, cfg_user_transform_init("tttttttttt", WL_OUTPUT_TRANSFORM_270));

	slist_append(&s->expected->order_name_desc, strdup("ooo"));
	slist_append(&s->expected->order_name_desc, strdup("oooooooooo"));

	slist_append(&s->expected->adaptive_sync_off_name_desc, strdup("vvv"));
	slist_append(&s->expected->adaptive_sync_off_name_desc, strdup("vvvvvvvvvv"));

	slist_append(&s->expected->max_preferred_refresh_name_desc, strdup("ppp"));
	slist_append(&s->expected->max_preferred_refresh_name_desc, strdup("pppppppppp"));

	slist_append(&s->expected->disabled, cfg_disabled_always("ddd"));
	slist_append(&s->expected->disabled, cfg_disabled_always("dddddddddd"));

	validate_warn(s->expected);

	char *expected_log = read_file("tst/cfg/validate-warn.log");
	assert_log(WARNING, expected_log);

	free(expected_log);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(merge_set__arrange),
		TEST(merge_set__align),
		TEST(merge_set__order),
		TEST(merge_set__auto_scale),
		TEST(merge_set__user_scale),
		TEST(merge_set__user_transform),
		TEST(merge_set__mode),
		TEST(merge_set__adaptive_sync_off),
		TEST(merge_set__disabled),
		TEST(merge_set__callback_cmd),

		TEST(merge_del__scale),
		TEST(merge_del__mode),
		TEST(merge_del__transform),
		TEST(merge_del__adaptive_sync_off),
		TEST(merge_del__disabled),
		TEST(merge_del__callback_cmd),

		TEST(merge_toggle__scaling),
		TEST(merge_toggle__auto_scale),
		TEST(merge_toggle__adaptive_sync_off),
		TEST(merge_toggle__disabled),

		TEST(validate_fix__col),
		TEST(validate_fix__row),
		TEST(validate_fix__scale),
		TEST(validate_fix__mode),
		TEST(validate_fix__disabled),

		TEST(validate_warn__),
	};

	return RUN(tests);
}

