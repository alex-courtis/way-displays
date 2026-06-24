#include "tst.h"

#include "assert-cfg.h"
#include "assert-log.h"
#include "util-file.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "cfg/disabled.h"
#include "cfg/user-mode.h"
#include "cfg/user-scale.h"
#include "conditions.h"
#include "log.h"
#include "pset.h"
#include "slist.h"
#include "smap.h"
#include "sset.h"

#include "cfg.h"

struct Cfg *merge_set(struct Cfg *to, const struct Cfg *from);
struct Cfg *merge_toggle(struct Cfg *to, const struct Cfg *from);
struct Cfg *merge_del(struct Cfg *to, const struct Cfg *from);


extern struct SList *cfg_file_paths;

struct State {
	struct Cfg *from;
	struct Cfg *to;
	struct Cfg *expected;
};

static int before_each(void **state) {
	struct State *s = calloc(1, sizeof(struct State));

	slist_free_vals(&cfg_file_paths, NULL);

	s->from = cfg_default();
	s->to = cfg_default();
	s->expected = cfg_default();

	*state = s;
	return 0;
}

static int after_each(void **state) {
	struct State *s = *state;

	slist_free_vals(&cfg_file_paths, NULL);

	cfg_destroy();

	cfg_free(s->from);
	cfg_free(s->to);
	cfg_free(s->expected);

	free(s);
	return 0;
}


static void merge_set__arrange(void **state) {
	struct State *s = *state;

	s->from->arrange = COL;
	s->expected->arrange = COL;

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_set__align(void **state) {
	struct State *s = *state;

	s->from->align = MIDDLE;
	s->expected->align = MIDDLE;

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_set__order(void **state) {
	struct State *s = *state;

	slist_append(&s->to->order_name_desc, strdup("A"));
	slist_append(&s->from->order_name_desc, strdup("X"));

	slist_append(&s->expected->order_name_desc, strdup("X"));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_set__auto_scale(void **state) {
	struct State *s = *state;

	s->from->auto_scale = OFF;
	s->expected->auto_scale = OFF;

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_set__scale_round_to(void **state) {
	struct State *s = *state;

	s->from->scale_round_to = 2;
	s->expected->scale_round_to = 2;

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_set__scale_round_strategy(void **state) {
	struct State *s = *state;

	s->from->scale_round_strategy = UP;
	s->expected->scale_round_strategy = UP;

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_set__user_scale(void **state) {
	struct State *s = *state;

	smap_put(s->to->user_scales, "to", user_scale_init("to", 1));
	smap_put(s->to->user_scales, "both", user_scale_init("both", 2));

	smap_put(s->from->user_scales, "from", user_scale_init("from", 3));
	smap_put(s->from->user_scales, "both", user_scale_init("both", 4));

	smap_put(s->expected->user_scales, "to", user_scale_init("to", 1));
	smap_put(s->expected->user_scales, "both", user_scale_init("both", 4));
	smap_put(s->expected->user_scales, "from", user_scale_init("from", 3));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_set__user_transform(void **state) {
	struct State *s = *state;

	smap_put(s->to->user_transforms, "to", cfg_user_transform_init(1));
	smap_put(s->to->user_transforms, "both", cfg_user_transform_init(2));

	smap_put(s->from->user_transforms, "from", cfg_user_transform_init(3));
	smap_put(s->from->user_transforms, "both", cfg_user_transform_init(4));

	smap_put(s->expected->user_transforms, "to", cfg_user_transform_init(1));
	smap_put(s->expected->user_transforms, "both", cfg_user_transform_init(4));
	smap_put(s->expected->user_transforms, "from", cfg_user_transform_init(3));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_set__mode(void **state) {
	struct State *s = *state;

	smap_put(s->to->user_modes, "to", user_mode_init("to", false, 1, 2, 3, false));
	smap_put(s->to->user_modes, "both", user_mode_init("both", false, 4, 5, 6, false));

	smap_put(s->from->user_modes, "from", user_mode_init("from", false, 7, 8, 9, true));
	smap_put(s->from->user_modes, "both", user_mode_init("both", false, 10, 11, 12, true));

	smap_put(s->expected->user_modes, "to", user_mode_init("to", false, 1, 2, 3, false));
	smap_put(s->expected->user_modes, "both", user_mode_init("both", false, 10, 11, 12, true));
	smap_put(s->expected->user_modes, "from", user_mode_init("from", false, 7, 8, 9, true));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_set__adaptive_sync_off(void **state) {
	struct State *s = *state;

	sset_add(s->to->adaptive_sync_off, "to");
	sset_add(s->to->adaptive_sync_off, "both");

	sset_add(s->from->adaptive_sync_off, "from");
	sset_add(s->from->adaptive_sync_off, "both");

	sset_add(s->expected->adaptive_sync_off, "to");
	sset_add(s->expected->adaptive_sync_off, "both");
	sset_add(s->expected->adaptive_sync_off, "from");

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_set__disabled(void **state) {
	struct State *s = *state;

	struct Disabled *disabled1 = calloc(1, sizeof(struct Disabled));
	disabled1->name_desc = strdup("cond");
	struct Condition *cond = calloc(1, sizeof(struct Condition));
	slist_append(&cond->plugged, strdup("display"));
	slist_append(&disabled1->conditions, cond);
	cond = calloc(1, sizeof(struct Condition));
	cond->lid = LID_NOT_PRESENT;
	slist_append(&disabled1->conditions, cond);

	struct Disabled *disabled2 = calloc(1, sizeof(struct Disabled));
	disabled2->name_desc = strdup("twelve");

	cond = calloc(1, sizeof(struct Condition));
	slist_append(&cond->plugged, strdup("FOUR"));
	slist_append(&disabled1->conditions, cond);

	assert_true(pset_add(s->to->disableds, disabled_init_always("to")));
	assert_true(pset_add(s->to->disableds, disabled_init_always("both")));

	assert_true(pset_add(s->from->disableds, disabled_init_always("from")));
	assert_true(pset_add(s->from->disableds, disabled_init_always("both")));
	assert_true(pset_add(s->from->disableds, disabled_clone(disabled1)));
	assert_true(pset_add(s->from->disableds, disabled_clone(disabled2)));

	assert_true(pset_add(s->expected->disableds, disabled_init_always("to")));
	assert_true(pset_add(s->expected->disableds, disabled_init_always("both")));
	assert_true(pset_add(s->expected->disableds, disabled_init_always("from")));
	assert_true(pset_add(s->expected->disableds, disabled1));
	assert_true(pset_add(s->expected->disableds, disabled2));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_set__callback_cmd(void **state) {
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

	assert_logs_empty();
}

static void merge_del__scale(void **state) {
	struct State *s = *state;

	smap_put(s->to->user_scales, "1", user_scale_init("1", 1));
	smap_put(s->to->user_scales, "2", user_scale_init("2", 2));

	smap_put(s->from->user_scales, "2", user_scale_init("2", 3));
	smap_put(s->from->user_scales, "3", user_scale_init("3", 4));

	smap_put(s->expected->user_scales, "1", user_scale_init("1", 1));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_del__mode(void **state) {
	struct State *s = *state;

	smap_put(s->to->user_modes, "1", user_mode_init("1", false, 1, 1, 1, false));
	smap_put(s->to->user_modes, "2", user_mode_init("2", false, 2, 2, 2, false));

	smap_put(s->from->user_modes, "2", user_mode_init("2", false, 2, 2, 2, false));
	smap_put(s->from->user_modes, "3", user_mode_init("3", false, 3, 3, 3, false));

	smap_put(s->from->user_modes, "1", user_mode_init("1", false, 1, 1, 1, false));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_del__transform(void **state) {
	struct State *s = *state;

	smap_put(s->to->user_transforms, "to", cfg_user_transform_init(1));
	smap_put(s->to->user_transforms, "both", cfg_user_transform_init(2));

	smap_put(s->from->user_transforms, "from", cfg_user_transform_init(3));
	smap_put(s->from->user_transforms, "both", cfg_user_transform_init(4));

	smap_put(s->expected->user_transforms, "to", cfg_user_transform_init(1));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_del__adaptive_sync_off(void **state) {
	struct State *s = *state;

	sset_add(s->to->adaptive_sync_off, "1");
	sset_add(s->to->adaptive_sync_off, "2");

	sset_add(s->from->adaptive_sync_off, "2");
	sset_add(s->from->adaptive_sync_off, "3");

	sset_add(s->expected->adaptive_sync_off, "1");

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_del__disabled(void **state) {
	struct State *s = *state;

	pset_add(s->to->disableds, disabled_init_always("1"));
	pset_add(s->to->disableds, disabled_init_always("2"));

	pset_add(s->from->disableds, disabled_init_always("2"));
	pset_add(s->from->disableds, disabled_init_always("3"));

	pset_add(s->expected->disableds, disabled_init_always("1"));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_del__callback_cmd(void **state) {
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

	assert_logs_empty();
}

static void merge_toggle__scaling(void **state) {
	struct State *s = *state;

	s->to->scaling = ON;

	s->from->scaling = ON;
	s->from->auto_scale = OFF;

	s->expected->scaling = OFF;

	struct Cfg *merged = merge_toggle(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_toggle__auto_scale(void **state) {
	struct State *s = *state;

	s->to->auto_scale = OFF;

	s->from->scaling = OFF;
	s->from->auto_scale = ON;

	s->expected->auto_scale = ON;

	struct Cfg *merged = merge_toggle(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_toggle__adaptive_sync_off(void **state) {
	struct State *s = *state;

	s->from->auto_scale = false;
	s->from->scaling = false;

	sset_add(s->to->adaptive_sync_off, "display1");
	sset_add(s->to->adaptive_sync_off, "display2");

	sset_add(s->from->adaptive_sync_off, "display2");
	sset_add(s->from->adaptive_sync_off, "display3");

	sset_add(s->expected->adaptive_sync_off, "display1");
	sset_add(s->expected->adaptive_sync_off, "display3");

	struct Cfg *merged = merge_toggle(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void validate_fix__col(void **state) {
	struct State *s = *state;

	s->from->arrange = COL;
	s->from->align = TOP;

	s->expected->arrange = COL;
	s->expected->align = LEFT;

	validate_fix(s->from);

	assert_log(WARNING, "\nIgnoring invalid ALIGN TOP for COLUMN arrange. Valid values are LEFT, MIDDLE and RIGHT. Using default LEFT.\n");
	assert_logs_empty();

	assert_cfg_equal(s->from, s->expected);
}

static void validate_fix__row(void **state) {
	struct State *s = *state;

	s->from->arrange = ROW;
	s->from->align = RIGHT;

	s->expected->arrange = ROW;
	s->expected->align = TOP;

	validate_fix(s->from);

	assert_log(WARNING, "\nIgnoring invalid ALIGN RIGHT for ROW arrange. Valid values are TOP, MIDDLE and BOTTOM. Using default TOP.\n");
	assert_logs_empty();

	assert_cfg_equal(s->from, s->expected);
}

static void validate_fix__user_scale(void **state) {
	struct State *s = *state;

	smap_put(s->from->user_scales, "ok", user_scale_init("ok", 1));

	smap_put(s->from->user_scales, "neg", user_scale_init("neg", -1));

	smap_put(s->from->user_scales, "zero", user_scale_init("zero", 0));

	smap_put(s->from->user_scales, "another", user_scale_init("dup", 3));

	validate_fix(s->from);

	char *expected_log = read_file("tst/cfg/validate-fix-user-scale.log");
	assert_log(WARNING, expected_log);
	assert_logs_empty();

	smap_put(s->expected->user_scales, "ok", user_scale_init("ok", 1));
	smap_put(s->expected->user_scales, "another", user_scale_init("dup", 3));

	assert_cfg_equal(s->from, s->expected);

	free(expected_log);
}

static void validate_fix__user_mode(void **state) {
	struct State *s = *state;

	smap_put(s->from->user_modes, "ok", user_mode_init("ok", false, 1, 2, 3, false));
	smap_put(s->from->user_modes, "max", user_mode_init("max", true, -1, -1, -1, false));

	smap_put(s->from->user_modes, "negative width", user_mode_init("negative width", false, -99, 2, 3, false));

	smap_put(s->from->user_modes, "negative height", user_mode_init("negative height", false, 1, -99, 3, false));

	smap_put(s->from->user_modes, "negative hz", user_mode_init("negative hz", false, 1, 2, -12340, false));

	smap_put(s->from->user_modes, "missing width", user_mode_init("missing width", false, -1, 2, 3, false));

	smap_put(s->from->user_modes, "missing height", user_mode_init("missing height", false, 1, -1, 3, false));

	validate_fix(s->from);

	char *expected_log = read_file("tst/cfg/validate-fix-user-mode.log");
	assert_log(WARNING, expected_log);
	assert_logs_empty();

	smap_put(s->expected->user_modes, "ok", user_mode_init("ok", false, 1, 2, 3, false));
	smap_put(s->expected->user_modes, "max", user_mode_init("max", true, -1, -1, -1, false));

	assert_cfg_equal(s->from, s->expected);

	free(expected_log);
}

static void validate_fix__auto_scale_dpi(void **state) {
	struct State *s = *state;

	s->from->auto_scale_dpi = -1;

	s->expected->auto_scale_dpi = 96;

	validate_fix(s->from);

	assert_log(WARNING, "\nIgnoring AUTO_SCALE_DPI -1 < 8. Using default 96.\n");
	assert_logs_empty();

	assert_cfg_equal(s->from, s->expected);
}

static void validate_warn__(void **state) {
	struct State *s = *state;

	smap_put(s->expected->user_scales, "sss", user_scale_init("sss", 1));
	smap_put(s->expected->user_scales, "ssssssss", user_scale_init("ssssssss", 2));
	smap_put(s->expected->user_scales, "DP-1", user_scale_init("DP-1", 3));

	smap_put(s->expected->user_modes, "mmm", user_mode_init("mmm", false, 1, 1, 1, false));
	smap_put(s->expected->user_modes, "mmmmmmmm", user_mode_init("mmmmmmmm", false, 1, 1, 1, false));
	smap_put(s->expected->user_modes, "DP-1", user_mode_init("DP-1", false, 1, 1, 1, false));

	smap_put(s->expected->user_transforms, "ttt", cfg_user_transform_init(WL_OUTPUT_TRANSFORM_180));
	smap_put(s->expected->user_transforms, "tttttttttt", cfg_user_transform_init(WL_OUTPUT_TRANSFORM_270));
	smap_put(s->expected->user_transforms, "DP-1", cfg_user_transform_init(WL_OUTPUT_TRANSFORM_270));

	slist_append(&s->expected->order_name_desc, strdup("ooo"));
	slist_append(&s->expected->order_name_desc, strdup("oooooooooo"));
	slist_append(&s->expected->order_name_desc, strdup("DP-1"));

	sset_add(s->expected->adaptive_sync_off, "vvv");
	sset_add(s->expected->adaptive_sync_off, "vvvvvvvvvv");
	sset_add(s->expected->adaptive_sync_off, "DP-1");

	slist_append(&s->expected->max_preferred_refresh_name_desc, strdup("ppp"));
	slist_append(&s->expected->max_preferred_refresh_name_desc, strdup("pppppppppp"));
	slist_append(&s->expected->max_preferred_refresh_name_desc, strdup("DP-1"));

	pset_add(s->expected->disableds, disabled_init_always("ddd"));
	pset_add(s->expected->disableds, disabled_init_always("dddddddddd"));
	pset_add(s->expected->disableds, disabled_init_always("DP-1"));

	struct Disabled *disabled = calloc(1, sizeof(struct Disabled));
	disabled->name_desc = strdup("cond");
	struct Condition *cond = calloc(1, sizeof(struct Condition));
	slist_append(&cond->plugged, strdup("ppp"));
	slist_append(&cond->plugged, strdup("DP-1"));
	slist_append(&cond->unplugged, strdup("uuu"));
	slist_append(&cond->unplugged, strdup("DP-1"));
	slist_append(&disabled->conditions, cond);

	pset_add(s->expected->disableds, disabled);

	validate_warn(s->expected);

	char *expected_log = read_file("tst/cfg/validate-warn.log");
	assert_log(WARNING, expected_log);
	assert_logs_empty();

	free(expected_log);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(merge_set__arrange),
		TEST_BA(merge_set__align),
		TEST_BA(merge_set__order),
		TEST_BA(merge_set__auto_scale),
		TEST_BA(merge_set__scale_round_to),
		TEST_BA(merge_set__scale_round_strategy),
		TEST_BA(merge_set__user_scale),
		TEST_BA(merge_set__user_transform),
		TEST_BA(merge_set__mode),
		TEST_BA(merge_set__adaptive_sync_off),
		TEST_BA(merge_set__disabled),
		TEST_BA(merge_set__callback_cmd),

		TEST_BA(merge_del__scale),
		TEST_BA(merge_del__mode),
		TEST_BA(merge_del__transform),
		TEST_BA(merge_del__adaptive_sync_off),
		TEST_BA(merge_del__disabled),
		TEST_BA(merge_del__callback_cmd),

		TEST_BA(merge_toggle__scaling),
		TEST_BA(merge_toggle__auto_scale),
		TEST_BA(merge_toggle__adaptive_sync_off),

		TEST_BA(validate_fix__col),
		TEST_BA(validate_fix__row),
		TEST_BA(validate_fix__user_scale),
		TEST_BA(validate_fix__user_mode),
		TEST_BA(validate_fix__auto_scale_dpi),

		TEST_BA(validate_warn__),
	};

	return RUN(tests);
}

