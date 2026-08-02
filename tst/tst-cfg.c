#include "tst.h"

#include "assert-cfg.h"
#include "assert-log.h"
#include "asserts.h"
#include "data.h"
#include "util-col.h"
#include "util-file.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "cfg/condition.h"
#include "cfg/disabled.h"
#include "enum.h"
#include "mode.h"
#include "pset.h"
#include "simap.h"
#include "spmap.h"
#include "sset.h"

#include "cfg/cfg.h"

struct State {
	struct Cfg *from;
	struct Cfg *to;
	struct Cfg *expected;
};

static int before_each(void **state) {
	struct State *s = calloc(1, sizeof(struct State));

	s->from = cfg_init();
	s->to = cfg_init();
	s->expected = cfg_init();

	*state = s;
	return 0;
}

static int after_each(void **state) {
	struct State *s = *state;

	g_cfg_destroy();

	cfg_free(s->from);
	cfg_free(s->to);
	cfg_free(s->expected);

	free(s);
	return 0;
}

static void cfg_equal__all(void **state) {
	struct Cfg *a = cfg_all();
	struct Cfg *b = cfg_all();

	assert_cfg_not_equal(a, NULL);
	assert_cfg_not_equal(NULL, b);

	a->align = MIDDLE;
	assert_cfg_not_equal(a, b);
	a->align = b->align;
	assert_cfg_equal(a, b);

	a->arrange = ROW;
	assert_cfg_not_equal(a, b);
	a->arrange = b->arrange;
	assert_cfg_equal(a, b);

	a->auto_scale = !a->auto_scale;
	assert_cfg_not_equal(a, b);
	a->auto_scale = b->auto_scale;
	assert_cfg_equal(a, b);

	a->auto_scale_dpi = !a->auto_scale_dpi;
	assert_cfg_not_equal(a, b);
	a->auto_scale_dpi = b->auto_scale_dpi;
	assert_cfg_equal(a, b);

	a->auto_scale_max = a->auto_scale_max + 1;
	assert_cfg_not_equal(a, b);
	a->auto_scale_max = b->auto_scale_max;
	assert_cfg_equal(a, b);

	a->auto_scale_min = a->auto_scale_min + 1;
	assert_cfg_not_equal(a, b);
	a->auto_scale_min = b->auto_scale_min;
	assert_cfg_equal(a, b);

	a->auto_scale_dpi = a->auto_scale_dpi + 1;
	assert_cfg_not_equal(a, b);
	a->auto_scale_dpi = b->auto_scale_dpi;
	assert_cfg_equal(a, b);

	a->laptop_lid_monitor = !a->laptop_lid_monitor;
	assert_cfg_not_equal(a, b);
	a->laptop_lid_monitor = b->laptop_lid_monitor;
	assert_cfg_equal(a, b);

	a->log_threshold = a->log_threshold + 1;
	assert_cfg_not_equal(a, b);
	a->log_threshold = b->log_threshold;
	assert_cfg_equal(a, b);

	a->scale_round_strategy = DOWN;
	assert_cfg_not_equal(a, b);
	a->scale_round_strategy = b->scale_round_strategy;
	assert_cfg_equal(a, b);

	a->scale_round_to = a->scale_round_to + 1;
	assert_cfg_not_equal(a, b);
	a->scale_round_to = b->scale_round_to;
	assert_cfg_equal(a, b);

	a->scaling = !a->scaling;
	assert_cfg_not_equal(a, b);
	a->scaling = b->scaling;
	assert_cfg_equal(a, b);

	free(a->callback_cmd);
	a->callback_cmd = strdup("foo");
	assert_cfg_not_equal(a, b);
	free(a->callback_cmd);
	a->callback_cmd = strdup(b->callback_cmd);
	assert_cfg_equal(a, b);

	free(a->laptop_display_prefix);
	a->laptop_display_prefix = strdup("foo");
	assert_cfg_not_equal(a, b);
	free(a->laptop_display_prefix);
	a->laptop_display_prefix = strdup(b->laptop_display_prefix);
	assert_cfg_equal(a, b);

	const struct CfgDisabled *disabled = cfg_disabled_init();
	spmap_put(a->disableds, "foo", disabled);
	assert_cfg_not_equal(a, b);
	spmap_remove_free(a->disableds, "foo");
	assert_cfg_equal(a, b);

	((struct Mode*)spmap_get(a->modes, "fifth"))->height = 9999999;
	assert_cfg_not_equal(a, b);
	((struct Mode*)spmap_get(a->modes, "fifth"))->height = 1080;
	assert_cfg_equal(a, b);

	simap_put(a->scales, "third", 999999);
	assert_cfg_not_equal(a, b);
	simap_put(a->scales, "third", 3000);
	assert_cfg_equal(a, b);

	simap_put(a->transforms, "twelfth", WL_OUTPUT_TRANSFORM_180);
	assert_cfg_not_equal(a, b);
	simap_put(a->transforms, "twelfth", WL_OUTPUT_TRANSFORM_FLIPPED);
	assert_cfg_equal(a, b);

	sset_add(a->adaptive_sync_off, "foo");
	assert_cfg_not_equal(a, b);
	sset_remove(a->adaptive_sync_off, "foo");
	assert_cfg_equal(a, b);

	sset_add(a->max_preferred_refresh, "foo");
	assert_cfg_not_equal(a, b);
	sset_remove(a->max_preferred_refresh, "foo");
	assert_cfg_equal(a, b);

	sset_add(a->order_name_desc, "foo");
	assert_cfg_not_equal(a, b);
	sset_remove(a->order_name_desc, "foo");
	assert_cfg_equal(a, b);

	cfg_free(a);
	cfg_free(b);

	assert_logs_empty();
}

static void cfg_clone__null(void **state) {
	assert_nul(cfg_clone(NULL));

	assert_logs_empty();
}

static void cfg_clone__empty(void **state) {
	struct Cfg *expected = cfg_init();

	struct Cfg *actual = cfg_clone(expected);

	assert_cfg_equal(actual, expected);

	cfg_free(expected);
	cfg_free(actual);

	assert_logs_empty();
}

static void cfg_clone__default(void **state) {
	struct Cfg *expected = cfg_default();

	struct Cfg *actual = cfg_clone(expected);

	assert_cfg_equal(actual, expected);

	cfg_free(expected);
	cfg_free(actual);

	assert_logs_empty();
}

static void cfg_clone__all(void **state) {
	struct Cfg *expected = cfg_all();

	struct Cfg *actual = cfg_clone(expected);

	assert_cfg_equal(actual, expected);

	cfg_free(expected);
	cfg_free(actual);

	assert_logs_empty();
}

static void cfg_apply_defaults__nop(void **state) {
	struct Cfg *expected = cfg_all();

	struct Cfg *actual = cfg_all();

	cfg_apply_defaults(actual);

	assert_cfg_equal(actual, expected);

	cfg_free(expected);
	cfg_free(actual);

	assert_logs_empty();
}

static void cfg_merge__bad_op(void **state) {
	struct Cfg *to = cfg_default();
	struct Cfg *from = cfg_all();

	assert_nul(cfg_merge(to, from, REAPPLY));

	cfg_free(to);
	cfg_free(from);

	assert_logs_empty();
}

static void cfg_merge__nop_set(void **state) {
	struct Cfg *to = cfg_default();
	struct Cfg *from = cfg_default();

	assert_nul(cfg_merge(to, NULL, CFG_SET));
	assert_nul(cfg_merge(NULL, from, CFG_SET));

	assert_nul(cfg_merge(to, from, CFG_SET));

	cfg_free(to);
	cfg_free(from);

	assert_logs_empty();
}

static void cfg_merge__nop_toggle(void **state) {
	struct Cfg *to = cfg_default();
	struct Cfg *from = cfg_init();

	assert_nul(cfg_merge(to, from, CFG_TOGGLE));

	cfg_free(to);
	cfg_free(from);

	assert_logs_empty();
}

static void cfg_merge__nop_del(void **state) {
	struct Cfg *to = cfg_default();
	struct Cfg *from = cfg_init();

	assert_nul(cfg_merge(to, from, CFG_DEL));

	cfg_free(to);
	cfg_free(from);

	assert_logs_empty();
}

static void cfg_merge__fix_set(void **state) {
	struct Cfg *to = cfg_default();
	struct Cfg *from = cfg_default();
	from->arrange = COL;
	from->align = TOP;

	struct Cfg *actual = cfg_merge(to, from, CFG_SET);

	assert_log(WARNING, "\nIgnoring invalid ALIGN TOP for COLUMN arrange. Valid values are LEFT, MIDDLE and RIGHT. Using default LEFT.\n");

	assert_cfg_not_equal(actual, from);
	assert_cfg_not_equal(actual, to);

	assert_int_equal(actual->arrange, COL);
	assert_int_equal(actual->align, LEFT);

	cfg_free(actual);
	cfg_free(to);
	cfg_free(from);

	assert_logs_empty();
}

static void cfg_merge_set__nulls(void **state) {
	struct State *s = *state;

	assert_nul(cfg_merge_set(s->to, NULL));
	assert_nul(cfg_merge_set(NULL, s->from));

	assert_logs_empty();
}

static void cfg_merge_set__no_changes(void **state) {
	struct Cfg *to = cfg_all();
	struct Cfg *from = cfg_all();
	struct Cfg *expected = cfg_all();

	struct Cfg *merged = cfg_merge_set(to, from);

	assert_cfg_equal(merged, expected);

	cfg_free(merged);
	cfg_free(from);
	cfg_free(to);
	cfg_free(expected);

	assert_logs_empty();
}

static void cfg_merge_set__all_changes(void **state) {
	struct Cfg *to = cfg_init();
	struct Cfg *from = cfg_all();
	struct Cfg *expected = cfg_all();

	// set the non-settable scalars
	to->scale_round_to = from->scale_round_to;
	to->scale_round_strategy = from->scale_round_strategy;
	to->auto_scale_dpi = from->auto_scale_dpi;
	to->auto_scale_min = from->auto_scale_min;
	to->auto_scale_max = from->auto_scale_max;
	to->log_threshold = from->log_threshold;
	to->laptop_lid_monitor = from->laptop_lid_monitor;
	if (from->laptop_display_prefix) {
		free(to->laptop_display_prefix);
		to->laptop_display_prefix = strdup(from->laptop_display_prefix);
	}

	struct Cfg *merged = cfg_merge_set(to, from);

	assert_cfg_equal(merged, expected);

	cfg_free(merged);
	cfg_free(from);
	cfg_free(to);
	cfg_free(expected);

	assert_logs_empty();
}

static void cfg_merge_set__arrange(void **state) {
	struct State *s = *state;

	s->from->arrange = COL;
	s->expected->arrange = COL;

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void cfg_merge_set__align(void **state) {
	struct State *s = *state;

	s->from->align = MIDDLE;
	s->expected->align = MIDDLE;

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void cfg_merge_set__order(void **state) {
	struct State *s = *state;

	sset_add(s->to->order_name_desc, "A");

	sset_add(s->from->order_name_desc, "X");

	sset_add(s->expected->order_name_desc, "X");

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void cfg_merge_set__scaling(void **state) {
	struct State *s = *state;

	s->from->scaling = OFF;
	s->expected->scaling = OFF;

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void cfg_merge_set__auto_scale(void **state) {
	struct State *s = *state;

	s->from->auto_scale = OFF;
	s->expected->auto_scale = OFF;

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void cfg_merge_set__scale(void **state) {
	struct State *s = *state;

	simap_put(s->to->scales, "to",   1000);
	simap_put(s->to->scales, "both", 2000);

	simap_put(s->from->scales, "from", 3000);
	simap_put(s->from->scales, "both", 4000);

	simap_put(s->expected->scales, "to",   1000);
	simap_put(s->expected->scales, "both", 4000);
	simap_put(s->expected->scales, "from", 3000);

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void cfg_merge_set__transform(void **state) {
	struct State *s = *state;

	simap_put(s->to->transforms, "to",   WL_OUTPUT_TRANSFORM_90);
	simap_put(s->to->transforms, "both", WL_OUTPUT_TRANSFORM_180);

	simap_put(s->from->transforms, "from", WL_OUTPUT_TRANSFORM_270);
	simap_put(s->from->transforms, "both", WL_OUTPUT_TRANSFORM_FLIPPED);

	simap_put(s->expected->transforms, "to",   WL_OUTPUT_TRANSFORM_90);
	simap_put(s->expected->transforms, "both", WL_OUTPUT_TRANSFORM_FLIPPED);
	simap_put(s->expected->transforms, "from", WL_OUTPUT_TRANSFORM_270);

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void cfg_merge_set__mode(void **state) {
	struct State *s = *state;

	spmap_put_many(s->to->modes,
			"to", mode_whr(1, 2, 3),
			"both", mode_whr(4, 5, 6),
			NULL);

	spmap_put_many(s->from->modes,
			"from", mode_whr(7, 8, 9),
			"both", mode_whr(10, 11, 12),
			NULL);

	spmap_put_many(s->expected->modes,
			"to", mode_whr(1, 2, 3),
			"both", mode_whr(10, 11, 12),
			"from", mode_whr(7, 8, 9),
			NULL);

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
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

	assert_logs_empty();
}

static void cfg_merge_set__disabled(void **state) {
	struct State *s = *state;

	struct CfgDisabled *disabled1 = cfg_disabled_init();

	struct CfgCondition *cond = cfg_condition_init();
	sset_add(cond->plugged, "display");
	pset_add(disabled1->conditions, cond);

	cond = cfg_condition_init();
	cond->lid = LID_NOT_PRESENT;
	pset_add(disabled1->conditions, cond);

	cond = cfg_condition_init();
	sset_add(cond->plugged, "FOUR");
	pset_add(disabled1->conditions, cond);

	spmap_put_many(s->to->disableds,
			"to", cfg_disabled_init(),
			"both", cfg_disabled_init(),
			NULL);

	spmap_put_many(s->from->disableds,
			"from", cfg_disabled_init(),
			"both", cfg_disabled_init(),
			"cond", cfg_disabled_clone(disabled1),
			NULL);

	spmap_put_many(s->expected->disableds,
			"to", cfg_disabled_init(),
			"both", cfg_disabled_init(),
			"from", cfg_disabled_init(),
			"cond", disabled1,
			NULL);

	struct Cfg *merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
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

	free(s->to->callback_cmd);
	s->to->callback_cmd = NULL;

	merged = cfg_merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void cfg_merge_del__nulls(void **state) {
	struct State *s = *state;

	assert_nul(cfg_merge_del(s->to, NULL));
	assert_nul(cfg_merge_del(NULL, s->from));

	assert_logs_empty();
}

static void cfg_merge_del__no_deletes(void **state) {
	struct Cfg *to = cfg_all();
	struct Cfg *from = cfg_init();
	struct Cfg *expected = cfg_all();

	struct Cfg *merged = cfg_merge_del(to, from);

	assert_cfg_equal(merged, expected);

	cfg_free(merged);
	cfg_free(from);
	cfg_free(to);
	cfg_free(expected);

	assert_logs_empty();
}

static void cfg_merge_del__all_deletes(void **state) {
	struct Cfg *to = cfg_all();
	struct Cfg *from = cfg_all();
	struct Cfg *expected = cfg_all();

	// remove all deletable
	spmap_remove_all_free(expected->disableds);
	spmap_remove_all_free(expected->modes);
	simap_remove_all(expected->scales);
	simap_remove_all(expected->transforms);
	sset_remove_all(expected->adaptive_sync_off);
	free(expected->callback_cmd);
	expected->callback_cmd = NULL;

	struct Cfg *merged = cfg_merge_del(to, from);

	assert_cfg_equal(merged, expected);

	cfg_free(merged);
	cfg_free(from);
	cfg_free(to);
	cfg_free(expected);

	assert_logs_empty();
}

static void cfg_merge_del__scale(void **state) {
	struct State *s = *state;

	simap_put(s->to->scales, "1", 1000);
	simap_put(s->to->scales, "2", 2000);

	simap_put(s->from->scales, "2", 3000);
	simap_put(s->from->scales, "3", 4000);

	simap_put(s->expected->scales, "1", 1000);

	struct Cfg *merged = cfg_merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void cfg_merge_del__mode(void **state) {
	struct State *s = *state;

	spmap_put_many(s->to->modes,
			"1", mode_whr(1, 1, 1),
			"2", mode_whr(2, 2, 2),
			NULL);

	spmap_put_many(s->from->modes,
			"2", mode_whr(2, 2, 2),
			"3", mode_whr(3, 3, 3),
			NULL);

	spmap_put_many(s->from->modes,
			"1", mode_whr(1, 1, 1),
			NULL);

	struct Cfg *merged = cfg_merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void cfg_merge_del__transform(void **state) {
	struct State *s = *state;

	simap_put(s->to->transforms, "to",   WL_OUTPUT_TRANSFORM_90);
	simap_put(s->to->transforms, "both", WL_OUTPUT_TRANSFORM_180);

	simap_put(s->from->transforms, "from", WL_OUTPUT_TRANSFORM_270);
	simap_put(s->from->transforms, "both", WL_OUTPUT_TRANSFORM_FLIPPED);

	simap_put(s->expected->transforms, "to", WL_OUTPUT_TRANSFORM_90);

	struct Cfg *merged = cfg_merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
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

	assert_logs_empty();
}

static void cfg_merge_del__disabled(void **state) {
	struct State *s = *state;

	spmap_put_many(s->to->disableds,
			"1", cfg_disabled_init(),
			"2", cfg_disabled_init(),
			NULL);

	spmap_put_many(s->from->disableds,
			"2", cfg_disabled_init(),
			"3", cfg_disabled_init(),
			NULL);

	spmap_put_many(s->expected->disableds,
			"1", cfg_disabled_init(),
			NULL);

	struct Cfg *merged = cfg_merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void cfg_merge_del__callback_cmd_any(void **state) {
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

	assert_logs_empty();
}

static void cfg_merge_toggle__nulls(void **state) {
	struct State *s = *state;

	assert_nul(cfg_merge_toggle(s->to, NULL));
	assert_nul(cfg_merge_toggle(NULL, s->from));

	assert_logs_empty();
}

static void cfg_merge_toggle__no_toggles(void **state) {
	struct Cfg *to = cfg_all();
	struct Cfg *from = cfg_init();
	struct Cfg *expected = cfg_all();

	struct Cfg *merged = cfg_merge_toggle(to, from);

	assert_cfg_equal(merged, expected);

	cfg_free(merged);
	cfg_free(from);
	cfg_free(to);
	cfg_free(expected);

	assert_logs_empty();
}

static void cfg_merge_toggle__all_toggles(void **state) {
	struct Cfg *to = cfg_all();
	struct Cfg *from = cfg_init();
	struct Cfg *expected = cfg_all();

	// change all togglable
	from->auto_scale = ON;
	from->scaling = ON;
	sset_add_all(from->adaptive_sync_off, to->adaptive_sync_off);

	expected->auto_scale = ON;
	expected->scaling = ON;
	sset_remove_all(expected->adaptive_sync_off);

	struct Cfg *merged = cfg_merge_toggle(to, from);

	assert_cfg_equal(merged, expected);

	cfg_free(merged);
	cfg_free(from);
	cfg_free(to);
	cfg_free(expected);

	assert_logs_empty();
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

	assert_logs_empty();
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

	assert_logs_empty();
}

static void cfg_merge_toggle__adaptive_sync_off(void **state) {
	struct State *s = *state;

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

	assert_logs_empty();
}

static void cfg_merge_toggle__disableds(void **state) {
	struct State *s = *state;

	spmap_put_many(s->to->disableds,
			"existing1", cfg_disabled_init(),
			"existing2", cfg_disabled_init(),
			NULL);

	spmap_put_many(s->from->disableds,
			"existing1", cfg_disabled_init(),
			"new1", cfg_disabled_init(),
			NULL);

	spmap_put_many(s->expected->disableds,
			"existing2", cfg_disabled_init(),
			"new1", cfg_disabled_init(),
			NULL);

	struct Cfg *merged = cfg_merge_toggle(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void cfg_validate_fix__null(void **state) {
	cfg_validate_fix(NULL);

	assert_logs_empty();
}

static void cfg_validate_fix__col_ok(void **state) {
	struct State *s = *state;

	cfg_apply_defaults(s->from);
	cfg_apply_defaults(s->expected);

	s->from->arrange = COL;
	s->expected->arrange = COL;

	s->from->align = LEFT;
	s->expected->align = LEFT;

	cfg_validate_fix(s->from);

	assert_cfg_equal(s->from, s->expected);

	s->from->align = MIDDLE;
	s->expected->align = MIDDLE;

	cfg_validate_fix(s->from);

	assert_cfg_equal(s->from, s->expected);

	s->from->align = RIGHT;
	s->expected->align = RIGHT;

	cfg_validate_fix(s->from);

	assert_cfg_equal(s->from, s->expected);

	assert_logs_empty();
}

static void cfg_validate_fix__col(void **state) {
	struct State *s = *state;

	cfg_apply_defaults(s->from);
	cfg_apply_defaults(s->expected);

	s->from->arrange = COL;
	s->expected->arrange = COL;

	s->from->align = TOP;
	s->expected->align = LEFT;

	cfg_validate_fix(s->from);

	assert_log(WARNING, "\nIgnoring invalid ALIGN TOP for COLUMN arrange. Valid values are LEFT, MIDDLE and RIGHT. Using default LEFT.\n");

	assert_cfg_equal(s->from, s->expected);

	assert_logs_empty();
}

static void cfg_validate_fix__row_ok(void **state) {
	struct State *s = *state;

	cfg_apply_defaults(s->from);
	cfg_apply_defaults(s->expected);

	s->from->arrange = ROW;
	s->expected->arrange = ROW;

	s->from->align = BOTTOM;
	s->expected->align = BOTTOM;

	cfg_validate_fix(s->from);

	assert_cfg_equal(s->from, s->expected);

	s->from->align = MIDDLE;
	s->expected->align = MIDDLE;

	cfg_validate_fix(s->from);

	assert_cfg_equal(s->from, s->expected);

	assert_logs_empty();
}

static void cfg_validate_fix__row(void **state) {
	struct State *s = *state;

	cfg_apply_defaults(s->from);
	cfg_apply_defaults(s->expected);

	s->from->arrange = ROW;
	s->expected->arrange = ROW;

	s->from->align = RIGHT;
	s->expected->align = TOP;

	cfg_validate_fix(s->from);

	assert_log(WARNING, "\nIgnoring invalid ALIGN RIGHT for ROW arrange. Valid values are TOP, MIDDLE and BOTTOM. Using default TOP.\n");

	assert_cfg_equal(s->from, s->expected);

	assert_logs_empty();
}

static void cfg_validate_fix__mode_cfg(void **state) {
	struct State *s = *state;

	cfg_apply_defaults(s->from);
	cfg_apply_defaults(s->expected);

	spmap_put_many(s->from->modes,
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

	spmap_put_many(s->expected->modes,
			"ok", mode_whr(1, 2, 3),
			"max", mode_whr_max(-1, -1, -1),
			NULL);

	assert_cfg_equal(s->from, s->expected);

	free(expected_log);

	assert_logs_empty();
}

static void cfg_validate_fix__auto_scale_dpi(void **state) {
	struct State *s = *state;

	cfg_apply_defaults(s->from);
	cfg_apply_defaults(s->expected);

	s->from->auto_scale_dpi = -1;

	s->expected->auto_scale_dpi = 96;

	cfg_validate_fix(s->from);

	assert_log(WARNING, "\nIgnoring AUTO_SCALE_DPI -1 < 8. Using default 96.\n");

	assert_cfg_equal(s->from, s->expected);

	assert_logs_empty();
}

static void cfg_validate_warn__(void **state) {
	const struct State *s = *state;

	cfg_validate_warn(NULL);

	simap_put(s->expected->scales, "sss",      1000);
	simap_put(s->expected->scales, "ssssssss", 2000);
	simap_put(s->expected->scales, "DP-1",     3000);

	spmap_put_many(s->expected->modes,
			"mmm", mode_whr(1, 1, 1),
			"mmmmmmmm", mode_whr(1, 1, 1),
			"DP-1", mode_whr(1, 1, 1),
			NULL);

	simap_put(s->expected->transforms, "ttt",        WL_OUTPUT_TRANSFORM_180);
	simap_put(s->expected->transforms, "tttttttttt", WL_OUTPUT_TRANSFORM_270);
	simap_put(s->expected->transforms, "DP-1",       WL_OUTPUT_TRANSFORM_270);

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

	struct CfgDisabled *disabled_cond = cfg_disabled_init();
	const struct CfgCondition *cond = cfg_condition_init();
	sset_add_many(cond->plugged, "ppp", "DP-1", NULL);
	sset_add_many(cond->unplugged, "uuu", "DP-1", NULL);
	pset_add(disabled_cond->conditions, cond);

	spmap_put_many(s->expected->disableds,
			"ddd", cfg_disabled_init(),
			"dddddddddd", cfg_disabled_init(),
			"DP-1", cfg_disabled_init(),
			"cond1", disabled_cond,
			NULL);

	cfg_validate_warn(s->expected);

	char *expected_log = read_file("tst/cfg/validate-warn.log");
	assert_log(WARNING, expected_log);

	free(expected_log);

	assert_logs_empty();
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(cfg_equal__all),

		TEST_BA(cfg_clone__null),
		TEST_BA(cfg_clone__empty),
		TEST_BA(cfg_clone__default),
		TEST_BA(cfg_clone__all),

		TEST_BA(cfg_apply_defaults__nop),

		TEST_BA(cfg_merge__bad_op),
		TEST_BA(cfg_merge__nop_set),
		TEST_BA(cfg_merge__nop_toggle),
		TEST_BA(cfg_merge__nop_del),
		TEST_BA(cfg_merge__fix_set),

		TEST_BA(cfg_merge_set__nulls),
		TEST_BA(cfg_merge_set__no_changes),
		TEST_BA(cfg_merge_set__all_changes),
		TEST_BA(cfg_merge_set__arrange),
		TEST_BA(cfg_merge_set__align),
		TEST_BA(cfg_merge_set__order),
		TEST_BA(cfg_merge_set__scaling),
		TEST_BA(cfg_merge_set__auto_scale),
		TEST_BA(cfg_merge_set__scale),
		TEST_BA(cfg_merge_set__transform),
		TEST_BA(cfg_merge_set__mode),
		TEST_BA(cfg_merge_set__adaptive_sync_off),
		TEST_BA(cfg_merge_set__disabled),
		TEST_BA(cfg_merge_set__callback_cmd),

		TEST_BA(cfg_merge_del__nulls),
		TEST_BA(cfg_merge_del__no_deletes),
		TEST_BA(cfg_merge_del__all_deletes),
		TEST_BA(cfg_merge_del__scale),
		TEST_BA(cfg_merge_del__mode),
		TEST_BA(cfg_merge_del__transform),
		TEST_BA(cfg_merge_del__adaptive_sync_off),
		TEST_BA(cfg_merge_del__disabled),
		TEST_BA(cfg_merge_del__callback_cmd_any),

		TEST_BA(cfg_merge_toggle__nulls),
		TEST_BA(cfg_merge_toggle__no_toggles),
		TEST_BA(cfg_merge_toggle__all_toggles),
		TEST_BA(cfg_merge_toggle__scaling),
		TEST_BA(cfg_merge_toggle__auto_scale),
		TEST_BA(cfg_merge_toggle__adaptive_sync_off),
		TEST_BA(cfg_merge_toggle__disableds),

		TEST_BA(cfg_validate_fix__null),

		TEST_BA(cfg_validate_fix__col_ok),
		TEST_BA(cfg_validate_fix__col),
		TEST_BA(cfg_validate_fix__row_ok),
		TEST_BA(cfg_validate_fix__row),
		TEST_BA(cfg_validate_fix__mode_cfg),

		TEST_BA(cfg_validate_fix__auto_scale_dpi),

		TEST_BA(cfg_validate_warn__),
	};

	return RUN(tests);
}

