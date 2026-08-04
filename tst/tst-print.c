#include "tst.h"

#include "assert-log.h"
#include "data.h"
#include "expects.h"
#include "util-col.h"
#include "util-file.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "cfg/cfg.h"
#include "cfg/condition.h"
#include "cfg/disabled.h"
#include "displ.h"
#include "enum.h"
#include "head.h"
#include "ipmap.h"
#include "output.h"
#include "ppmap.h"
#include "pset.h"
#include "simap.h"
#include "spmap.h"
#include "sset.h"
#include "wlr-output-management-unstable-v1.h"

#include "info/print.h"

struct State {
	struct Head *head1;
	struct Head *head2;
	const struct PPmap *heads;
};

int before_each(void **state) {
	struct State *s = calloc(1, sizeof(struct State));

	g_displ = displ_init();

	g_cfg = cfg_default();

	s->heads = head_ppmap_init();

	s->head1 = head_init();

	ppmap_put_many(s->head1->modes,
			M0, mode_whr(100, 200, 29999), // less than current
			MC, mode_whr(100, 200, 30000), // current
			M1, mode_whr(100, 200, 30001), // more than current
			MD, mode_whr(400, 500, 60000), // desired
			M2, mode_whr(700, 800, 89999), // group with failed
			M3, mode_whr(700, 800, 90001), // end
			M4, mode_whr(1000, 1000, 49499),
			M5, mode_whr(1000, 1000, 49500), // group start
			M6, mode_whr(1000, 1000, 49999), //
			M7, mode_whr(1000, 1000, 50000), //
			M8, mode_whr(1000, 1000, 50100), //
			M9, mode_whr(1000, 1000, 50499), // group
			M10, mode_whr(1000, 1000, 50500),
			NULL);

	s->head1->cur.zmode = MC;
	s->head1->des.zmode = MD;
	s->head1->zmode_pref = MC;

	ppmap_put_many(s->head1->modes_failed,
			MF, mode_whr(700, 800, 90000),
			NULL);

	s->head1->name = strdup("name1");
	s->head1->description = strdup("description1");
	s->head1->width_mm = 1;
	s->head1->height_mm = 2;
	s->head1->make = strdup("make1");
	s->head1->model = strdup("model1");
	s->head1->serial_number = strdup("serial_number1");

	s->head1->cur.scale = 512;
	s->head1->cur.enabled = true;
	s->head1->cur.x = 700;
	s->head1->cur.y = 800;
	s->head1->cur.transform = WL_OUTPUT_TRANSFORM_180;
	s->head1->cur.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	s->head1->des.scale = 1024;
	s->head1->des.enabled = true;
	s->head1->des.x = 900;
	s->head1->des.y = 1000;
	s->head1->des.transform = WL_OUTPUT_TRANSFORM_90;
	s->head1->des.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	ppmap_put(s->heads, H1, s->head1);


	s->head2 = head_init();

	ppmap_put_many(s->head2->modes,
			MC, mode_whr(1100, 1200, 130000), // current
			MD, mode_whr(1400, 1500, 160000), // desired
			NULL);
	s->head2->cur.zmode = MC;
	s->head2->des.zmode = MD;

	s->head2->name = strdup("name2");
	s->head2->width_mm = 3;
	s->head2->height_mm = 4;
	s->head2->make = strdup("make2");
	s->head2->model = strdup("model2");
	s->head2->serial_number = strdup("serial_number2");

	s->head2->cur.scale = 2048;
	s->head2->cur.enabled = true;
	s->head2->cur.x = 1700;
	s->head2->cur.y = 1800;
	s->head2->cur.transform = WL_OUTPUT_TRANSFORM_270;
	s->head2->cur.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;

	s->head2->des.scale = 4096;
	s->head2->des.enabled = true;
	s->head2->des.x = 1900;
	s->head2->des.y = 11000;
	s->head2->des.transform = WL_OUTPUT_TRANSFORM_NORMAL;
	s->head2->des.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	ppmap_put(s->heads, H2, s->head2);

	*state = s;
	return 0;
}

int after_each(void **state) {
	struct State *s = *state;

	ppmap_free_vals(s->heads);

	free(s);

	displ_free(g_displ);

	g_cfg_destroy();

	return 0;
}

static void print_cfg__all(void **state) {
	struct Cfg *c = cfg_default();

	sset_add_many(c->order_name_desc,
			"first",
			"last",
			NULL);

	simap_put(c->scales, "three", 3000);
	simap_put(c->scales, "four",  4000);

	spmap_put(c->disableds, "disabled always", cfg_disabled_init());

	const struct CfgDisabled *disabled = cfg_disabled_init();
	const struct CfgCondition *cond = cfg_condition_init();
	sset_add(cond->plugged, "ONE");
	pset_add(disabled->conditions, cond);
	spmap_put(c->disableds, "disabled conditionally", disabled);

	spmap_put_many(c->modes,
			"five", mode_whr(1920, 1080, 12340),
			"six", mode_whr(2560, 1440, -1),
			"seven", mode_whr_max(1, 2, 3),
			"eight", mode_whr_max_pref(4, 5, 6),
			NULL);

	simap_put(c->transforms, "twelve", WL_OUTPUT_TRANSFORM_FLIPPED);

	sset_add(c->max_preferred_refresh, "legacy");

	c->laptop_display_prefix = strdup("lappy");

	c->scale_round_to = 2;
	c->scale_round_strategy = DOWN;

	print_cfg(INFO, c, false);

	char *expected_log = read_file("tst/info/print-cfg-all.log");
	assert_log(INFO, expected_log);

	free(expected_log);
	cfg_free(c);

	assert_logs_empty();
}

static void print_cfg__del(void **state) {
	struct Cfg *c = cfg_init();

	simap_put(c->scales, "three", 3000);
	simap_put(c->scales, "four",  4000);

	spmap_put_many(c->modes,
			"five", mode_whr(1920, 1080, 12340),
			"six", mode_whr(2560, 1440, -1),
			"seven", mode_whr_max(-1, -1, -1),
			NULL);

	simap_put(c->transforms, "twelve",   WL_OUTPUT_TRANSFORM_FLIPPED);
	simap_put(c->transforms, "thirteen", WL_OUTPUT_TRANSFORM_FLIPPED);

	print_cfg(INFO, c, true);

	char *expected_log = read_file("tst/info/print-cfg-del.log");
	assert_log(INFO, expected_log);

	free(expected_log);
	cfg_free(c);

	assert_logs_empty();
}

static void print_cfg__arrange_only(void **state) {
	struct Cfg *c = cfg_init();
	c->arrange = ROW;

	print_cfg(INFO, c, false);

	char *expected_log = read_file("tst/info/print-cfg-arrange-only.log");
	assert_log(INFO, expected_log);

	free(expected_log);
	cfg_free(c);

	assert_logs_empty();
}

static void print_cfg__align_only(void **state) {
	struct Cfg *c = cfg_init();
	c->align = TOP;

	print_cfg(INFO, c, false);

	char *expected_log = read_file("tst/info/print-cfg-align-only.log");
	assert_log(INFO, expected_log);

	free(expected_log);
	cfg_free(c);

	assert_logs_empty();
}

static void print_cfg__auto_scale_max(void **state) {
	struct Cfg *c = cfg_init();
	c->auto_scale = true;
	c->auto_scale_dpi = 77;
	c->auto_scale_min = 88.0f;
	c->auto_scale_max = 99.0f;

	print_cfg(INFO, c, false);

	char *expected_log = read_file("tst/info/print-cfg-auto-scale-max.log");
	assert_log(INFO, expected_log);

	free(expected_log);
	cfg_free(c);

	assert_logs_empty();
}

static void print_cfg__lid_disabled(void **state) {
	struct Cfg *c = cfg_init();
	c->laptop_lid_monitor = OFF;

	print_cfg(INFO, c, false);

	char *expected_log = read_file("tst/info/print-cfg-lid-disabled.log");
	assert_log(INFO, expected_log);

	free(expected_log);
	cfg_free(c);

	assert_logs_empty();
}

static void print_cfg_commands__empty(void **state) {
	struct Cfg *cfg = cfg_init();

	print_cfg_commands(INFO, cfg);

	cfg_free(cfg);

	assert_logs_empty();
}

static void print_cfg_commands__ok(void **state) {
	struct Cfg *c = cfg_default();

	c->arrange = COL;
	c->align = RIGHT;

	sset_add_many(c->order_name_desc,
			"one",
			"two",
			"three",
			NULL);

	c->scaling = OFF;

	c->auto_scale = OFF;

	simap_put(c->scales, "one", 1000);
	simap_put(c->scales, "two", 2345);

	spmap_put_many(c->modes,
			"all", mode_whr(1, 2, 12340),
			"res", mode_whr(4, 5, -1),
			"max", mode_whr_max(7, 8, 9),
			"max_pref", mode_whr_max_pref(10, 11, 12),
			NULL);

	simap_put(c->transforms, "seven", WL_OUTPUT_TRANSFORM_FLIPPED_90);

	spmap_put_many(c->disableds,
			"three", cfg_disabled_init(),
			"four", cfg_disabled_init(),
			NULL);

	sset_add_many(c->adaptive_sync_off,
			"five",
			"six",
			NULL);

	print_cfg_commands(INFO, c);

	char *expected_log = read_file("tst/info/print-cfg-commands-ok.log");
	assert_log(INFO, expected_log);

	cfg_free(c);
	free(expected_log);

	assert_logs_empty();
}

static void print_head_arrived__all(void **state) {
	const struct State *s = *state;

	expect_str(__wrap_g_lid_is_closed, name, "name1");
	will_return_int(__wrap_g_lid_is_closed, false);

	ipmap_put(g_displ->outputs, 888, output_n("inexistent"));

	struct Output *output1 = output_n("name1");
	output1->logical_width = 4000;
	output1->logical_height = 2000;
	output1->logical_x = 400;
	output1->logical_y = 200;
	ipmap_put(g_displ->outputs, 999, output1);

	print_head(INFO, ARRIVED, s->head1);

	char *expected_log = read_file("tst/info/print-head-arrived-all.log");
	assert_log(INFO, expected_log);
	free(expected_log);

	assert_logs_empty();
}

static void print_head_arrived__min(void **state) {
	struct Head *head = head_init();

	expect_str(__wrap_g_lid_is_closed, name, NULL);
	will_return_int(__wrap_g_lid_is_closed, false);

	print_head(INFO, ARRIVED, head);

	char *expected_log = read_file("tst/info/print-head-arrived-min.log");
	assert_log(INFO, expected_log);
	free(expected_log);

	head_free(head);

	assert_logs_empty();
}

static void print_head_departed__ok(void **state) {
	const struct State *s = *state;

	print_head(INFO, DEPARTED, s->head1);

	char *expected_log = read_file("tst/info/print-head-departed-ok.log");
	assert_log(INFO, expected_log);
	free(expected_log);

	assert_logs_empty();
}

static void print_head_deltas__mode(void **state) {
	const struct State *s = *state;

	expect_str(__wrap_g_lid_is_closed, name, "name1");
	will_return_int(__wrap_g_lid_is_closed, false);

	print_head(INFO, DELTA, s->head1);

	char *expected_log = read_file("tst/info/print-head-deltas-mode.log");
	assert_log(INFO, expected_log);
	free(expected_log);

	assert_logs_empty();
}

static void print_head_deltas__vrr(void **state) {
	struct State *s = *state;

	s->head1->des.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	s->head1->des.zmode = s->head1->cur.zmode;

	expect_str(__wrap_g_lid_is_closed, name, "name1");
	will_return_int(__wrap_g_lid_is_closed, false);

	print_head(INFO, DELTA, s->head1);

	char *expected_log = read_file("tst/info/print-head-deltas-vrr.log");
	assert_log(INFO, expected_log);
	free(expected_log);

	assert_logs_empty();
}

static void print_head_deltas__other(void **state) {
	struct State *s = *state;

	s->head1->des.zmode = s->head1->cur.zmode;

	expect_str(__wrap_g_lid_is_closed, name, "name1");
	will_return_int(__wrap_g_lid_is_closed, false);

	print_head(INFO, DELTA, s->head1);

	char *expected_log = read_file("tst/info/print-head-deltas-other.log");
	assert_log(INFO, expected_log);
	free(expected_log);

	assert_logs_empty();
}

static void print_head_deltas__disable(void **state) {
	struct State *s = *state;

	s->head1->des.enabled = false;

	expect_str(__wrap_g_lid_is_closed, name, "name1");
	will_return_int(__wrap_g_lid_is_closed, false);

	print_head(INFO, DELTA, s->head1);

	char *expected_log = read_file("tst/info/print-head-deltas-disable.log");
	assert_log(INFO, expected_log);
	free(expected_log);

	assert_logs_empty();
}

static void print_head_deltas__enable(void **state) {
	struct State *s = *state;

	s->head1->cur.enabled = false;

	expect_str(__wrap_g_lid_is_closed, name, "name1");
	will_return_int(__wrap_g_lid_is_closed, false);

	print_head(INFO, DELTA, s->head1);

	char *expected_log = read_file("tst/info/print-head-deltas-enable.log");
	assert_log(INFO, expected_log);
	free(expected_log);

	assert_logs_empty();
}

static void print_head_deltas__reapply(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;
	head.des = head.cur;
	head.cur.enabled = false;
	head.des.enabled = false;
	head.reapply_required = true;

	expect_str(__wrap_g_lid_is_closed, name, "name1");
	will_return_int(__wrap_g_lid_is_closed, false);

	print_head(INFO, DELTA, &head);

	char *expected_log = read_file("tst/info/print-head-deltas-reapply.log");
	assert_log(INFO, expected_log);
	free(expected_log);

	assert_logs_empty();
}

static void print_head_current__disabled(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;
	head.cur.enabled = false;

	expect_str(__wrap_g_lid_is_closed, name, "name1");
	will_return_int(__wrap_g_lid_is_closed, false);

	print_head_current(INFO, &head);

	char *expected_log = read_file("tst/info/print-head-current-disabled.log");
	assert_log(INFO, expected_log);
	free(expected_log);

	assert_logs_empty();
}

static void print_head_current__disabled_override(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;
	head.cur.enabled = false;
	head.overrided_enabled = OverrideFalse;

	expect_str(__wrap_g_lid_is_closed, name, "name1");
	will_return_int(__wrap_g_lid_is_closed, false);

	print_head_current(INFO, &head);

	char *expected_log = read_file("tst/info/print-head-current-disabled-override.log");
	assert_log(INFO, expected_log);
	free(expected_log);

	assert_logs_empty();
}

static void print_head_current__enabled_override(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;
	head.cur.enabled = true;
	head.overrided_enabled = OverrideTrue;

	expect_str(__wrap_g_lid_is_closed, name, "name1");
	will_return_int(__wrap_g_lid_is_closed, false);

	print_head_current(INFO, &head);

	char *expected_log = read_file("tst/info/print-head-current-enabled-override.log");
	assert_log(INFO, expected_log);
	free(expected_log);

	assert_logs_empty();
}

static void print_head_current__lid_closed(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;

	expect_str(__wrap_g_lid_is_closed, name, "name1");
	will_return_int(__wrap_g_lid_is_closed, true);

	print_head_current(INFO, &head);

	char *expected_log = read_file("tst/info/print-head-current-lid-closed.log");
	assert_log(INFO, expected_log);
	free(expected_log);

	assert_logs_empty();
}

static void print_head_desired__disabled(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;
	head.des.enabled = false;

	print_head_desired(INFO, &head);

	assert_log(INFO, "    (disabled)\n");

	assert_logs_empty();
}

static void print_head_desired__disabled_override(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;
	head.des.enabled = false;
	head.overrided_enabled = OverrideFalse;

	print_head_desired(INFO, &head);

	assert_log(INFO, "    (manually disabled)\n");

	assert_logs_empty();
}

static void print_head_desired__enabled(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;
	head.cur.enabled = false;
	head.des.enabled = true;

	print_head_desired(INFO, &head);

	assert_log(INFO, "    mode:      400x500@60Hz (60,000mHz)\n    (enabled)\n");

	assert_logs_empty();
}

static void print_head_desired__enabled_override(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;
	head.cur.enabled = false;
	head.des.enabled = true;
	head.overrided_enabled = OverrideTrue;

	print_head_desired(INFO, &head);

	assert_log(INFO, "    mode:      400x500@60Hz (60,000mHz)\n    (manually enabled)\n");

	assert_logs_empty();
}

static void print_head_desired__transform_270(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;
	memcpy(&head.des, &head.cur, sizeof(struct HeadState));
	head.des.transform = WL_OUTPUT_TRANSFORM_270;

	print_head_desired(INFO, &head);

	assert_log(INFO, "    transform: 270\n");

	assert_logs_empty();
}

static void print_head_desired__transform_none(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;
	memcpy(&head.des, &head.cur, sizeof(struct HeadState));
	head.des.transform = 0;

	print_head_desired(INFO, &head);

	assert_log(INFO, "    transform: none\n");

	assert_logs_empty();
}

static void print_list__empty(void **state) {
	print_list(INFO, NULL);

	assert_logs_empty();
}

static void print_list__many(void **state) {
	struct State *s = *state;

	s->head1->cur.enabled = false;
	print_list(INFO, s->heads);

	char *expected_log = read_file("tst/info/print-list.log");
	assert_log(INFO, expected_log);
	free(expected_log);

	assert_logs_empty();
}

static void print_adaptive_sync_fail__nulls(void **state) {
	print_adaptive_sync_fail(ERROR, NULL);

	assert_logs_empty();
}

static void print_adaptive_sync_fail__head(void **state) {
	struct Head *head = head_init();
	head->name = strdup("head0");
	head->model = strdup("model0");

	print_adaptive_sync_fail(WARNING, head);

	assert_log(WARNING, "\nhead0:\n"
			"  Cannot enable VRR: this display or compositor may not support it.\n"
			"  To speed things up you can disable VRR for this display by adding the following or similar to your cfg.yaml\n"
			"  VRR_OFF:\n"
			"    - 'model0'\n");

	head_free(head);

	assert_logs_empty();
}

static void print_mode_fail__nulls(void **state) {

	print_mode_fail(WARNING, NULL, NULL);

	assert_log(WARNING, "\nChanges failed\n");

	assert_logs_empty();
}

static void print_mode_fail__head(void **state) {
	struct Head *head = head_init();
	head->name = strdup("head0");
	head->model = strdup("model0");

	print_mode_fail(WARNING, head, NULL);

	assert_log(WARNING, "\nChanges failed\n  head0:\n    (no mode)\n");

	head_free(head);

	assert_logs_empty();
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(print_cfg__all),
		TEST_BA(print_cfg__arrange_only),
		TEST_BA(print_cfg__align_only),
		TEST_BA(print_cfg__auto_scale_max),
		TEST_BA(print_cfg__del),
		TEST_BA(print_cfg__lid_disabled),

		TEST_BA(print_cfg_commands__empty),
		TEST_BA(print_cfg_commands__ok),

		TEST_BA(print_head_arrived__all),
		TEST_BA(print_head_arrived__min),
		TEST_BA(print_head_departed__ok),

		TEST_BA(print_head_deltas__mode),
		TEST_BA(print_head_deltas__vrr),
		TEST_BA(print_head_deltas__other),
		TEST_BA(print_head_deltas__disable),
		TEST_BA(print_head_deltas__enable),
		TEST_BA(print_head_deltas__reapply),

		TEST_BA(print_head_current__disabled),
		TEST_BA(print_head_current__disabled_override),
		TEST_BA(print_head_current__enabled_override),

		TEST_BA(print_head_current__lid_closed),

		TEST_BA(print_head_desired__disabled),
		TEST_BA(print_head_desired__disabled_override),
		TEST_BA(print_head_desired__enabled),
		TEST_BA(print_head_desired__enabled_override),

		TEST_BA(print_head_desired__transform_270),
		TEST_BA(print_head_desired__transform_none),

		TEST_BA(print_list__empty),
		TEST_BA(print_list__many),

		TEST_BA(print_adaptive_sync_fail__nulls),
		TEST_BA(print_adaptive_sync_fail__head),

		TEST_BA(print_mode_fail__nulls),
		TEST_BA(print_mode_fail__head),
	};

	return RUN(tests);
}

