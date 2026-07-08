#include "tst.h"

#include "assert-cfg.h"
#include "assert-log.h"
#include "util-col.h"
#include "util-file.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "cfg/condition.h"
#include "cfg/disabled.h"
#include "log.h"
#include "pset.h"
#include "slist.h"
#include "smap.h"
#include "sset.h"

#include "cfg.h"

extern struct SList *g_cfg_file_paths;

struct State {
	struct Cfg *from;
	struct Cfg *to;
	struct Cfg *expected;
};

static int before_each(void **state) {
	struct State *s = calloc(1, sizeof(struct State));

	slist_free_vals(&g_cfg_file_paths, NULL);

	s->from = cfg_default();
	s->to = cfg_default();
	s->expected = cfg_default();

	*state = s;
	return 0;
}

static int after_each(void **state) {
	assert_logs_empty();

	struct State *s = *state;

	slist_free_vals(&g_cfg_file_paths, NULL);

	cfg_destroy();

	cfg_free(s->from);
	cfg_free(s->to);
	cfg_free(s->expected);

	free(s);
	return 0;
}

static void cfg_equal__mode(void **state) {
	const struct State *s = *state;

	smap_put(s->from->modes, "both", mode_whr(4, 5, 6));

	smap_put(s->to->modes, "both", mode_whr(10, 11, 12));

	assert_cfg_not_equal(s->from, s->to);
}

static void cfg_merge_set__arrange(void **state) {
	struct State *s = *state;

	s->from->arrange = COL;
	s->expected->arrange = COL;

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_merge_set__align(void **state) {
	struct State *s = *state;

	s->from->align = MIDDLE;
	s->expected->align = MIDDLE;

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_merge_set__order(void **state) {
	struct State *s = *state;

	sset_add(s->to->order_name_desc, "A");

	sset_add(s->from->order_name_desc, "X");

	sset_add(s->expected->order_name_desc, "X");

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_merge_set__auto_scale(void **state) {
	struct State *s = *state;

	s->from->auto_scale = OFF;
	s->expected->auto_scale = OFF;

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_merge_set__scale_round_to(void **state) {
	struct State *s = *state;

	s->from->scale_round_to = 2;
	s->expected->scale_round_to = 2;

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_merge_set__scale_round_strategy(void **state) {
	struct State *s = *state;

	s->from->scale_round_strategy = UP;
	s->expected->scale_round_strategy = UP;

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_merge_set__scale(void **state) {
	struct State *s = *state;

	smapi_put_many(s->to->scales,
			"to", 1000,
			"both", 2000,
			NULL);

	smapi_put_many(s->from->scales,
			"from", 3000,
			"both", 4000,
			NULL);

	smapi_put_many(s->expected->scales,
			"to", 1000,
			"both", 4000,
			"from", 3000,
			NULL);

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_merge_set__transform(void **state) {
	struct State *s = *state;

	smapi_put_many(s->to->transforms,
			"to", 1000,
			"both", 2000,
			NULL);

	smapi_put_many(s->from->transforms,
			"from", 3000,
			"both", 4000,
			NULL);

	smapi_put_many(s->expected->transforms,
			"to", 1000,
			"both", 4000,
			"from", 3000,
			NULL);

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_merge_set__mode(void **state) {
	struct State *s = *state;

	smap_put_many(s->to->modes,
			"to", mode_whr(1, 2, 3),
			"both", mode_whr(4, 5, 6),
			NULL);

	smap_put_many(s->from->modes,
			"from", mode_whr(7, 8, 9),
			"both", mode_whr(10, 11, 12),
			NULL);

	smap_put_many(s->expected->modes,
			"to", mode_whr(1, 2, 3),
			"both", mode_whr(10, 11, 12),
			"from", mode_whr(7, 8, 9),
			NULL);

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_merge_set__adaptive_sync_off(void **state) {
	struct State *s = *state;

	sset_add_many(s->to->adaptive_sync_off,
			"to",
			"both",
			NULL);

	sset_add_many(s->from->adaptive_sync_off,
			"from",
			"both",
			NULL);

	sset_add_many(s->expected->adaptive_sync_off,
			"to",
			"both",
			"from",
			NULL);

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_merge_set__disabled(void **state) {
	struct State *s = *state;

	struct Disabled *disabled1 = disabled_nd("cond");

	struct Condition *cond = condition_init();
	sset_add(cond->plugged, "display");
	pset_add(disabled1->conditions, cond);

	cond = condition_init();
	cond->lid = LID_NOT_PRESENT;
	pset_add(disabled1->conditions, cond);

	cond = condition_init();
	sset_add(cond->plugged, "FOUR");
	pset_add(disabled1->conditions, cond);

	pset_add_many(s->to->disableds,
			disabled_nd("to"),
			disabled_nd("both"),
			NULL);

	pset_add_many(s->from->disableds,
			disabled_nd("from"),
			disabled_nd("both"),
			disabled_clone(disabled1),
			NULL);

	pset_add_many(s->expected->disableds,
			disabled_nd("to"),
			disabled_nd("both"),
			disabled_nd("from"),
			disabled1,
			NULL);

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_merge_set__callback_cmd(void **state) {
	struct State *s = *state;

	free(s->to->callback_cmd);
	s->to->callback_cmd = strdup("to");

	free(s->from->callback_cmd);
	s->from->callback_cmd = strdup("from");

	free(s->expected->callback_cmd);
	s->expected->callback_cmd = strdup("from");

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_merge_del__scale(void **state) {
	struct State *s = *state;

	smapi_put_many(s->to->scales,
			"1", 1000,
			"2", 2000,
			NULL);

	smapi_put_many(s->from->scales,
			"2", 3000,
			"3", 4000,
			NULL);

	smapi_put_many(s->expected->scales,
			"1", 1000,
			NULL);

	struct Cfg *merged = cfg_merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_merge_del__mode(void **state) {
	struct State *s = *state;

	smap_put_many(s->to->modes,
			"1", mode_whr(1, 1, 1),
			"2", mode_whr(2, 2, 2),
			NULL);

	smap_put_many(s->from->modes,
			"2", mode_whr(2, 2, 2),
			"3", mode_whr(3, 3, 3),
			NULL);

	smap_put_many(s->from->modes,
			"1", mode_whr(1, 1, 1),
			NULL);

	struct Cfg *merged = cfg_merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_merge_del__transform(void **state) {
	struct State *s = *state;

	smapi_put_many(s->to->transforms,
			"to", 1,
			"both", 2,
			NULL);

	smapi_put_many(s->from->transforms,
			"from", 3,
			"both", 4,
			NULL);

	smapi_put_many(s->expected->transforms,
			"to", 1,
			NULL);

	struct Cfg *merged = cfg_merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_merge_del__adaptive_sync_off(void **state) {
	struct State *s = *state;

	sset_add_many(s->to->adaptive_sync_off,
			"1",
			"2",
			NULL);

	sset_add_many(s->from->adaptive_sync_off,
			"2",
			"3",
			NULL);

	sset_add_many(s->expected->adaptive_sync_off,
			"1",
			NULL);

	struct Cfg *merged = cfg_merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_merge_del__disabled(void **state) {
	struct State *s = *state;

	pset_add_many(s->to->disableds,
			disabled_nd("1"),
			disabled_nd("2"),
			NULL);

	pset_add_many(s->from->disableds,
			disabled_nd("2"),
			disabled_nd("3"),
			NULL);

	pset_add_many(s->expected->disableds,
			disabled_nd("1"),
			NULL);

	struct Cfg *merged = cfg_merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_merge_del__callback_cmd(void **state) {
	struct State *s = *state;

	free(s->to->callback_cmd);
	s->to->callback_cmd = strdup("to");

	free(s->from->callback_cmd);
	s->from->callback_cmd = strdup("");

	free(s->expected->callback_cmd);
	s->expected->callback_cmd = NULL;

	struct Cfg *merged = cfg_merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_merge_toggle__scaling(void **state) {
	struct State *s = *state;

	s->to->scaling = ON;

	s->from->scaling = ON;
	s->from->auto_scale = OFF;

	s->expected->scaling = OFF;

	struct Cfg *merged = cfg_merge_toggle(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_merge_toggle__auto_scale(void **state) {
	struct State *s = *state;

	s->to->auto_scale = OFF;

	s->from->scaling = OFF;
	s->from->auto_scale = ON;

	s->expected->auto_scale = ON;

	struct Cfg *merged = cfg_merge_toggle(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_merge_toggle__adaptive_sync_off(void **state) {
	struct State *s = *state;

	s->from->auto_scale = false;
	s->from->scaling = false;

	sset_add_many(s->to->adaptive_sync_off,
			"display1",
			"display2",
			NULL);

	sset_add_many(s->from->adaptive_sync_off,
			"display2",
			"display3",
			NULL);

	sset_add_many(s->expected->adaptive_sync_off,
			"display1",
			"display3",
			NULL);

	struct Cfg *merged = cfg_merge_toggle(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

static void cfg_validate_fix__col(void **state) {
	struct State *s = *state;

	s->from->arrange = COL;
	s->from->align = TOP;

	s->expected->arrange = COL;
	s->expected->align = LEFT;

	cfg_validate_fix(s->from);

	assert_log(WARNING, "\nIgnoring invalid ALIGN TOP for COLUMN arrange. Valid values are LEFT, MIDDLE and RIGHT. Using default LEFT.\n");

	assert_cfg_equal(s->from, s->expected);
}

static void cfg_validate_fix__row(void **state) {
	struct State *s = *state;

	s->from->arrange = ROW;
	s->from->align = RIGHT;

	s->expected->arrange = ROW;
	s->expected->align = TOP;

	cfg_validate_fix(s->from);

	assert_log(WARNING, "\nIgnoring invalid ALIGN RIGHT for ROW arrange. Valid values are TOP, MIDDLE and BOTTOM. Using default TOP.\n");

	assert_cfg_equal(s->from, s->expected);
}

static void cfg_validate_fix__mode_cfg(void **state) {
	struct State *s = *state;

	// TODO could this just be put_many ?
	smap_put_many(s->from->modes,
			"ok", mode_whr(1, 2, 3),
			"max", mode_whr_max(-1, -1, -1),
			"negative width", mode_whr(-99, 2, 3),
			"negative height", mode_whr(1, -99, 3),
			"negative hz", mode_whr(1, 2, -12340),
			"missing width", mode_whr(-1, 2, 3),
			"missing height", mode_whr(1, -1, 3),
			NULL);

	cfg_validate_fix(s->from);

	char *expected_log = read_file("tst/cfg/validate-fix-mode.log");
	assert_log(WARNING, expected_log);

	smap_put_many(s->expected->modes,
			"ok", mode_whr(1, 2, 3),
			"max", mode_whr_max(-1, -1, -1),
			NULL);

	assert_cfg_equal(s->from, s->expected);

	free(expected_log);
}

static void cfg_validate_fix__auto_scale_dpi(void **state) {
	struct State *s = *state;

	s->from->auto_scale_dpi = -1;

	s->expected->auto_scale_dpi = 96;

	cfg_validate_fix(s->from);

	assert_log(WARNING, "\nIgnoring AUTO_SCALE_DPI -1 < 8. Using default 96.\n");

	assert_cfg_equal(s->from, s->expected);
}

static void cfg_validate_warn__(void **state) {
	const struct State *s = *state;

	smapi_put_many(s->expected->scales,
			"sss", 1000,
			"ssssssss", 2000,
			"DP-1", 3000,
			NULL);

	smap_put_many(s->expected->modes,
			"mmm", mode_whr(1, 1, 1),
			"mmmmmmmm", mode_whr(1, 1, 1),
			"DP-1", mode_whr(1, 1, 1),
			NULL);

	smapi_put_many(s->expected->transforms,
			"ttt", WL_OUTPUT_TRANSFORM_180,
			"tttttttttt", WL_OUTPUT_TRANSFORM_270,
			"DP-1", WL_OUTPUT_TRANSFORM_270,
			NULL);

	sset_add_many(s->expected->order_name_desc,
			"ooo",
			"oooooooooo",
			"DP-1",
			NULL);

	sset_add_many(s->expected->adaptive_sync_off,
			"vvv",
			"vvvvvvvvvv",
			"DP-1",
			NULL);

	sset_add_many(s->expected->max_preferred_refresh,
			"ppp",
			"pppppppppp",
			"DP-1",
			NULL);

	struct Disabled *disabled_cond = disabled_nd("cond1");
	const struct Condition *cond = condition_init();
	sset_add_many(cond->plugged, "ppp", "DP-1", NULL);
	sset_add_many(cond->unplugged, "uuu", "DP-1", NULL);
	pset_add(disabled_cond->conditions, cond);

	pset_add_many(s->expected->disableds,
			disabled_nd("ddd"),
			disabled_nd("dddddddddd"),
			disabled_nd("DP-1"),
			disabled_cond,
			NULL);

	cfg_validate_warn(s->expected);

	char *expected_log = read_file("tst/cfg/validate-warn.log");
	assert_log(WARNING, expected_log);

	free(expected_log);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(cfg_equal__mode),

		TEST_BA(cfg_merge_set__arrange),
		TEST_BA(cfg_merge_set__align),
		TEST_BA(cfg_merge_set__order),
		TEST_BA(cfg_merge_set__auto_scale),
		TEST_BA(cfg_merge_set__scale_round_to),
		TEST_BA(cfg_merge_set__scale_round_strategy),
		TEST_BA(cfg_merge_set__scale),
		TEST_BA(cfg_merge_set__transform),
		TEST_BA(cfg_merge_set__mode),
		TEST_BA(cfg_merge_set__adaptive_sync_off),
		TEST_BA(cfg_merge_set__disabled),
		TEST_BA(cfg_merge_set__callback_cmd),

		TEST_BA(cfg_merge_del__scale),
		TEST_BA(cfg_merge_del__mode),
		TEST_BA(cfg_merge_del__transform),
		TEST_BA(cfg_merge_del__adaptive_sync_off),
		TEST_BA(cfg_merge_del__disabled),
		TEST_BA(cfg_merge_del__callback_cmd),

		TEST_BA(cfg_merge_toggle__scaling),
		TEST_BA(cfg_merge_toggle__auto_scale),
		TEST_BA(cfg_merge_toggle__adaptive_sync_off),

		TEST_BA(cfg_validate_fix__col),
		TEST_BA(cfg_validate_fix__row),
		TEST_BA(cfg_validate_fix__mode_cfg),
		TEST_BA(cfg_validate_fix__auto_scale_dpi),

		TEST_BA(cfg_validate_warn__),
	};

	return RUN(tests);
}

