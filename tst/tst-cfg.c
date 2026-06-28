#include "tst.h"

#include "assert-cfg.h"
#include "assert-log.h"
#include "util-file.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "cfg/condition.h"
#include "cfg/disabled.h"
#include "cfg/user-mode.h"
#include "log.h"
#include "pset.h"
#include "slist.h"
#include "smap.h"
#include "smapi.h"
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
	assert_logs_empty_before();

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

	sset_add(s->to->order_name_desc, "A");
	sset_add(s->from->order_name_desc, "X");

	sset_add(s->expected->order_name_desc, "X");

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

static void merge_set__scale(void **state) {
	struct State *s = *state;

	smapi_put(s->to->scales, "to", 1000);
	smapi_put(s->to->scales, "both", 2000);

	smapi_put(s->from->scales, "from", 3000);
	smapi_put(s->from->scales, "both", 4000);

	smapi_put(s->expected->scales, "to", 1000);
	smapi_put(s->expected->scales, "both", 4000);
	smapi_put(s->expected->scales, "from", 3000);

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_set__transform(void **state) {
	struct State *s = *state;

	smapi_put(s->to->transforms, "to", 1000);
	smapi_put(s->to->transforms, "both", 2000);

	smapi_put(s->from->transforms, "from", 3000);
	smapi_put(s->from->transforms, "both", 4000);

	smapi_put(s->expected->transforms, "to", 1000);
	smapi_put(s->expected->transforms, "both", 4000);
	smapi_put(s->expected->transforms, "from", 3000);

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_set__mode(void **state) {
	struct State *s = *state;

	smap_put(s->to->user_modes, "to", user_mode_init(false, 1, 2, 3, false));
	smap_put(s->to->user_modes, "both", user_mode_init(false, 4, 5, 6, false));

	smap_put(s->from->user_modes, "from", user_mode_init(false, 7, 8, 9, true));
	smap_put(s->from->user_modes, "both", user_mode_init(false, 10, 11, 12, true));

	smap_put(s->expected->user_modes, "to", user_mode_init(false, 1, 2, 3, false));
	smap_put(s->expected->user_modes, "both", user_mode_init(false, 10, 11, 12, true));
	smap_put(s->expected->user_modes, "from", user_mode_init(false, 7, 8, 9, true));

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

	struct Disabled *disabled1 = disabled_init();
	disabled1->name_desc = strdup("cond");
	struct Condition *cond = condition_init();
	sset_add(cond->plugged, "display");
	pset_add(disabled1->conditions, cond);
	cond = condition_init();
	cond->lid = LID_NOT_PRESENT;
	pset_add(disabled1->conditions, cond);

	struct Disabled *disabled2 = disabled_init();
	disabled2->name_desc = strdup("twelve");

	cond = condition_init();
	sset_add(cond->plugged, "FOUR");
	pset_add(disabled1->conditions, cond);

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

	smapi_put(s->to->scales, "1", 1000);
	smapi_put(s->to->scales, "2", 2000);

	smapi_put(s->from->scales, "2", 3000);
	smapi_put(s->from->scales, "3", 4000);

	smapi_put(s->expected->scales, "1", 1000);

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_del__mode(void **state) {
	struct State *s = *state;

	smap_put(s->to->user_modes, "1", user_mode_init(false, 1, 1, 1, false));
	smap_put(s->to->user_modes, "2", user_mode_init(false, 2, 2, 2, false));

	smap_put(s->from->user_modes, "2", user_mode_init(false, 2, 2, 2, false));
	smap_put(s->from->user_modes, "3", user_mode_init(false, 3, 3, 3, false));

	smap_put(s->from->user_modes, "1", user_mode_init(false, 1, 1, 1, false));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);

	assert_logs_empty();
}

static void merge_del__transform(void **state) {
	struct State *s = *state;

	smapi_put(s->to->transforms, "to", 1);
	smapi_put(s->to->transforms, "both", 2);

	smapi_put(s->from->transforms, "from", 3);
	smapi_put(s->from->transforms, "both", 4);

	smapi_put(s->expected->transforms, "to", 1);

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

static void validate_fix__user_mode(void **state) {
	struct State *s = *state;

	smap_put(s->from->user_modes, "ok", user_mode_init(false, 1, 2, 3, false));
	smap_put(s->from->user_modes, "max", user_mode_init(true, -1, -1, -1, false));

	smap_put(s->from->user_modes, "negative width", user_mode_init(false, -99, 2, 3, false));

	smap_put(s->from->user_modes, "negative height", user_mode_init(false, 1, -99, 3, false));

	smap_put(s->from->user_modes, "negative hz", user_mode_init(false, 1, 2, -12340, false));

	smap_put(s->from->user_modes, "missing width", user_mode_init(false, -1, 2, 3, false));

	smap_put(s->from->user_modes, "missing height", user_mode_init(false, 1, -1, 3, false));

	validate_fix(s->from);

	char *expected_log = read_file("tst/cfg/validate-fix-user-mode.log");
	assert_log(WARNING, expected_log);
	assert_logs_empty();

	smap_put(s->expected->user_modes, "ok", user_mode_init(false, 1, 2, 3, false));
	smap_put(s->expected->user_modes, "max", user_mode_init(true, -1, -1, -1, false));

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
	const struct State *s = *state;

	smapi_put(s->expected->scales, "sss", 1000);
	smapi_put(s->expected->scales, "ssssssss", 2000);
	smapi_put(s->expected->scales, "DP-1", 3000);

	smap_put(s->expected->user_modes, "mmm", user_mode_init(false, 1, 1, 1, false));
	smap_put(s->expected->user_modes, "mmmmmmmm", user_mode_init(false, 1, 1, 1, false));
	smap_put(s->expected->user_modes, "DP-1", user_mode_init(false, 1, 1, 1, false));

	smapi_put(s->expected->transforms, "ttt", WL_OUTPUT_TRANSFORM_180);
	smapi_put(s->expected->transforms, "tttttttttt", WL_OUTPUT_TRANSFORM_270);
	smapi_put(s->expected->transforms, "DP-1", WL_OUTPUT_TRANSFORM_270);

	sset_add(s->expected->order_name_desc, "ooo");
	sset_add(s->expected->order_name_desc, "oooooooooo");
	sset_add(s->expected->order_name_desc, "DP-1");

	sset_add(s->expected->adaptive_sync_off, "vvv");
	sset_add(s->expected->adaptive_sync_off, "vvvvvvvvvv");
	sset_add(s->expected->adaptive_sync_off, "DP-1");

	sset_add(s->expected->max_preferred_refresh, "ppp");
	sset_add(s->expected->max_preferred_refresh, "pppppppppp");
	sset_add(s->expected->max_preferred_refresh, "DP-1");

	pset_add(s->expected->disableds, disabled_init_always("ddd"));
	pset_add(s->expected->disableds, disabled_init_always("dddddddddd"));
	pset_add(s->expected->disableds, disabled_init_always("DP-1"));

	struct Disabled *disabled = disabled_init();
	disabled->name_desc = strdup("cond");
	const struct Condition *cond = condition_init();
	sset_add(cond->plugged, "ppp");
	sset_add(cond->plugged, "DP-1");
	sset_add(cond->unplugged, "uuu");
	sset_add(cond->unplugged, "DP-1");
	pset_add(disabled->conditions, cond);

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
		TEST_BA(merge_set__scale),
		TEST_BA(merge_set__transform),
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
		TEST_BA(validate_fix__user_mode),
		TEST_BA(validate_fix__auto_scale_dpi),

		TEST_BA(validate_warn__),
	};

	return RUN(tests);
}

