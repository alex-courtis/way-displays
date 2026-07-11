#include "tst.h"

#include "assert-log.h"
#include "expects.h"
#include "util-col.h"
#include "util-file.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "cfg.h"
#include "cfg/condition.h"
#include "cfg/disabled.h"
#include "displ.h"
#include "fn.h"
#include "head.h"
#include "imap.h"
#include "log.h"
#include "mode.h"
#include "output.h"
#include "pset.h"
#include "pslist.h"
#include "sset.h"
#include "wlr-output-management-unstable-v1.h"

#include "info/print.h"

struct State {
	struct Head *head1;
	struct Head *head2;
	struct Pslist *heads;
};

int before_each(void **state) {
	struct State *s = calloc(1, sizeof(struct State));

	g_cfg = cfg_default();

	s->head1 = head_init();

	s->head1->current.mode = mode_h_whr(s->head1, 100, 200, 30000);
	s->head1->mode_preferred = s->head1->current.mode;

	s->head1->desired.mode = mode_h_whr(s->head1, 400, 500, 60000);

	pset_add_many(s->head1->modes,
			mode_h_whr(s->head1, 100, 200, 29999), // less than current
			s->head1->current.mode,
			mode_h_whr(s->head1, 100, 200, 30001), // more than current
			s->head1->desired.mode,
			mode_h_whr(s->head1, 700, 800, 89999), // group with failed
			mode_h_whr(s->head1, 700, 800, 90001), // end
			mode_h_whr(s->head1, 1000, 1000, 49499),
			mode_h_whr(s->head1, 1000, 1000, 49500), // group start
			mode_h_whr(s->head1, 1000, 1000, 49999), //
			mode_h_whr(s->head1, 1000, 1000, 50000), //
			mode_h_whr(s->head1, 1000, 1000, 50100), //
			mode_h_whr(s->head1, 1000, 1000, 50499), // group
			mode_h_whr(s->head1, 1000, 1000, 50500),
			NULL);

	pset_add_many(s->head1->modes_failed,
			mode_h_whr(s->head1, 700, 800, 90000),
			NULL);

	s->head1->name = strdup("name1");
	s->head1->description = strdup("description1");
	s->head1->width_mm = 1;
	s->head1->height_mm = 2;
	s->head1->make = strdup("make1");
	s->head1->model = strdup("model1");
	s->head1->serial_number = strdup("serial_number1");

	s->head1->current.scale = 512;
	s->head1->current.enabled = true;
	s->head1->current.x = 700;
	s->head1->current.y = 800;
	s->head1->current.transform = WL_OUTPUT_TRANSFORM_180;
	s->head1->current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	s->head1->desired.scale = 1024;
	s->head1->desired.enabled = true;
	s->head1->desired.x = 900;
	s->head1->desired.y = 1000;
	s->head1->desired.transform = WL_OUTPUT_TRANSFORM_90;
	s->head1->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	pslist_append(&s->heads, s->head1);


	s->head2 = head_init();

	s->head2->current.mode = mode_h_whr(s->head2, 1100, 1200, 130000);
	s->head2->desired.mode = mode_h_whr(s->head2, 1400, 1500, 160000);

	pset_add(s->head2->modes, s->head2->current.mode);
	pset_add(s->head2->modes, s->head2->desired.mode);

	s->head2->name = strdup("name2");
	s->head2->width_mm = 3;
	s->head2->height_mm = 4;
	s->head2->make = strdup("make2");
	s->head2->model = strdup("model2");
	s->head2->serial_number = strdup("serial_number2");

	s->head2->current.scale = 2048;
	s->head2->current.enabled = true;
	s->head2->current.x = 1700;
	s->head2->current.y = 1800;
	s->head2->current.transform = WL_OUTPUT_TRANSFORM_270;
	s->head2->current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;

	s->head2->desired.scale = 4096;
	s->head2->desired.enabled = true;
	s->head2->desired.x = 1900;
	s->head2->desired.y = 11000;
	s->head2->desired.transform = WL_OUTPUT_TRANSFORM_NORMAL;
	s->head2->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	pslist_append(&s->heads, s->head2);

	g_outputs = imap_init();

	*state = s;
	return 0;
}

int after_each(void **state) {
	assert_logs_empty();

	struct State *s = *state;

	pslist_free_vals(&s->heads, (fn_free)head_free);

	free(s);

	imap_free_vals(g_outputs);
	g_outputs = NULL;

	cfg_destroy();

	return 0;
}

static void print_cfg__all(void **state) {
	struct Cfg *c = cfg_default();

	sset_add_many(c->order_name_desc,
			"first",
			"last",
			NULL);

	smapi_put_many(c->scales,
			"three", 3000,
			"four", 4000,
			NULL);

	pset_add(c->disableds, disabled_nd("disabled always"));
	const struct Disabled *disabled = disabled_nd("disabled conditionally");
	const struct Condition *cond = condition_init();
	sset_add(cond->plugged, "ONE");
	pset_add(disabled->conditions, cond);
	pset_add(c->disableds, disabled);

	smap_put_many(c->modes,
			"five", mode_whr(1920, 1080, 12340),
			"six", mode_whr(2560, 1440, -1),
			"seven", mode_whr_max(-1, -1, -1),
			NULL);

	smapi_put_many(c->transforms,
			"twelve", WL_OUTPUT_TRANSFORM_FLIPPED,
			NULL);

	sset_add(c->max_preferred_refresh, "legacy");

	c->laptop_display_prefix = strdup("lappy");

	c->scale_round_to = 2;
	c->scale_round_strategy = DOWN;

	print_cfg(INFO, c, false);

	char *expected_log = read_file("tst/info/print-cfg-all.log");
	assert_log(INFO, expected_log);

	free(expected_log);
	cfg_free(c);
}

static void print_cfg__del(void **state) {
	struct Cfg *c = cfg_init();

	smapi_put_many(c->scales,
			"three", 3000,
			"four", 4000,
			NULL);

	smap_put_many(c->modes,
			"five", mode_whr(1920, 1080, 12340),
			"six", mode_whr(2560, 1440, -1),
			"seven", mode_whr_max(-1, -1, -1),
			NULL);

	smapi_put_many(c->transforms,
			"twelve", WL_OUTPUT_TRANSFORM_FLIPPED,
			"thirteen", WL_OUTPUT_TRANSFORM_FLIPPED,
			NULL);

	print_cfg(INFO, c, true);

	char *expected_log = read_file("tst/info/print-cfg-del.log");
	assert_log(INFO, expected_log);

	free(expected_log);
	cfg_free(c);
}

static void print_cfg__arrange_only(void **state) {
	struct Cfg *c = cfg_init();
	c->arrange = ROW;

	print_cfg(INFO, c, false);

	char *expected_log = read_file("tst/info/print-cfg-arrange-only.log");
	assert_log(INFO, expected_log);

	free(expected_log);
	cfg_free(c);
}

static void print_cfg__align_only(void **state) {
	struct Cfg *c = cfg_init();
	c->align = TOP;

	print_cfg(INFO, c, false);

	char *expected_log = read_file("tst/info/print-cfg-align-only.log");
	assert_log(INFO, expected_log);

	free(expected_log);
	cfg_free(c);
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
}

static void print_cfg__lid_disabled(void **state) {
	struct Cfg *c = cfg_init();
	c->laptop_lid_monitor = OFF;

	print_cfg(INFO, c, false);

	char *expected_log = read_file("tst/info/print-cfg-lid-disabled.log");
	assert_log(INFO, expected_log);

	free(expected_log);
	cfg_free(c);
}

static void print_cfg_commands__empty(void **state) {
	struct Cfg *cfg = cfg_init();

	print_cfg_commands(INFO, cfg);

	cfg_free(cfg);
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

	smapi_put_many(c->scales,
			"one", 1000,
			"two", 2345,
			NULL);

	smap_put_many(c->modes,
			"all", mode_whr(1, 2, 12340),
			"res", mode_whr(4, 5, -1),
			"max", mode_whr_max(7, 8, 9),
			NULL);

	smapi_put_many(c->transforms,
			"seven", WL_OUTPUT_TRANSFORM_FLIPPED_90,
			NULL);

	pset_add_many(c->disableds,
			disabled_nd("three"),
			disabled_nd("four"),
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
}

static void print_head_arrived__all(void **state) {
	const struct State *s = *state;

	expect_str(__wrap_lid_is_closed, name, "name1");
	will_return_int(__wrap_lid_is_closed, false);

	struct Output *outputX = calloc(1, sizeof(struct Output));
	outputX->name = "inexistent"; // we don't call output destroy, just free
	imap_put(g_outputs, 888, outputX);

	struct Output *output1 = calloc(1, sizeof(struct Output));
	output1->name = "name1";
	output1->logical_width = 4000;
	output1->logical_height = 2000;
	output1->logical_x = 400;
	output1->logical_y = 200;
	imap_put(g_outputs, 999, output1);

	print_head(INFO, ARRIVED, s->head1);

	char *expected_log = read_file("tst/info/print-head-arrived-all.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

static void print_head_arrived__min(void **state) {
	struct Head *head = head_init();

	expect_str(__wrap_lid_is_closed, name, NULL);
	will_return_int(__wrap_lid_is_closed, false);

	print_head(INFO, ARRIVED, head);

	char *expected_log = read_file("tst/info/print-head-arrived-min.log");
	assert_log(INFO, expected_log);
	free(expected_log);

	head_free(head);
}

static void print_head_departed__ok(void **state) {
	const struct State *s = *state;

	print_head(INFO, DEPARTED, s->head1);

	char *expected_log = read_file("tst/info/print-head-departed-ok.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

static void print_head_deltas__mode(void **state) {
	const struct State *s = *state;

	expect_str(__wrap_lid_is_closed, name, "name1");
	will_return_int(__wrap_lid_is_closed, false);

	print_head(INFO, DELTA, s->head1);

	char *expected_log = read_file("tst/info/print-head-deltas-mode.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

static void print_head_deltas__vrr(void **state) {
	struct State *s = *state;

	s->head1->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	s->head1->desired.mode = s->head1->current.mode;

	expect_str(__wrap_lid_is_closed, name, "name1");
	will_return_int(__wrap_lid_is_closed, false);

	print_head(INFO, DELTA, s->head1);

	char *expected_log = read_file("tst/info/print-head-deltas-vrr.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

static void print_head_deltas__other(void **state) {
	struct State *s = *state;

	s->head1->desired.mode = s->head1->current.mode;

	expect_str(__wrap_lid_is_closed, name, "name1");
	will_return_int(__wrap_lid_is_closed, false);

	print_head(INFO, DELTA, s->head1);

	char *expected_log = read_file("tst/info/print-head-deltas-other.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

static void print_head_deltas__disable(void **state) {
	struct State *s = *state;

	s->head1->desired.enabled = false;

	expect_str(__wrap_lid_is_closed, name, "name1");
	will_return_int(__wrap_lid_is_closed, false);

	print_head(INFO, DELTA, s->head1);

	char *expected_log = read_file("tst/info/print-head-deltas-disable.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

static void print_head_deltas__enable(void **state) {
	struct State *s = *state;

	s->head1->current.enabled = false;

	expect_str(__wrap_lid_is_closed, name, "name1");
	will_return_int(__wrap_lid_is_closed, false);

	print_head(INFO, DELTA, s->head1);

	char *expected_log = read_file("tst/info/print-head-deltas-enable.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

static void print_head_deltas__reapply(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;
	head.desired = head.current;
	head.current.enabled = false;
	head.desired.enabled = false;
	head.reapply_required = true;

	expect_str(__wrap_lid_is_closed, name, "name1");
	will_return_int(__wrap_lid_is_closed, false);

	print_head(INFO, DELTA, &head);

	char *expected_log = read_file("tst/info/print-head-deltas-reapply.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

static void print_head_current__disabled(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;
	head.current.enabled = false;

	expect_str(__wrap_lid_is_closed, name, "name1");
	will_return_int(__wrap_lid_is_closed, false);

	print_head_current(INFO, &head);

	char *expected_log = read_file("tst/info/print-head-current-disabled.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

static void print_head_current__disabled_override(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;
	head.current.enabled = false;
	head.overrided_enabled = OverrideFalse;

	expect_str(__wrap_lid_is_closed, name, "name1");
	will_return_int(__wrap_lid_is_closed, false);

	print_head_current(INFO, &head);

	char *expected_log = read_file("tst/info/print-head-current-disabled-override.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

static void print_head_current__enabled_override(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;
	head.current.enabled = true;
	head.overrided_enabled = OverrideTrue;

	expect_str(__wrap_lid_is_closed, name, "name1");
	will_return_int(__wrap_lid_is_closed, false);

	print_head_current(INFO, &head);

	char *expected_log = read_file("tst/info/print-head-current-enabled-override.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

static void print_head_current__lid_closed(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;

	expect_str(__wrap_lid_is_closed, name, "name1");
	will_return_int(__wrap_lid_is_closed, true);

	print_head_current(INFO, &head);

	char *expected_log = read_file("tst/info/print-head-current-lid-closed.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

static void print_head_desired__disabled(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;
	head.desired.enabled = false;

	print_head_desired(INFO, &head);

	assert_log(INFO, "    (disabled)\n");
}

static void print_head_desired__disabled_override(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;
	head.desired.enabled = false;
	head.overrided_enabled = OverrideFalse;

	print_head_desired(INFO, &head);

	assert_log(INFO, "    (manually disabled)\n");
}

static void print_head_desired__enabled(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;
	head.current.enabled = false;
	head.desired.enabled = true;

	print_head_desired(INFO, &head);

	assert_log(INFO, "    mode:      400x500@60Hz (60,000mHz)\n    (enabled)\n");
}

static void print_head_desired__enabled_override(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;
	head.current.enabled = false;
	head.desired.enabled = true;
	head.overrided_enabled = OverrideTrue;

	print_head_desired(INFO, &head);

	assert_log(INFO, "    mode:      400x500@60Hz (60,000mHz)\n    (manually enabled)\n");
}

static void print_head_desired__transform_270(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;
	memcpy(&head.desired, &head.current, sizeof(struct HeadState));
	head.desired.transform = WL_OUTPUT_TRANSFORM_270;

	print_head_desired(INFO, &head);

	assert_log(INFO, "    transform: 270\n");
}

static void print_head_desired__transform_none(void **state) {
	struct State *s = *state;

	struct Head head = *s->head1;
	memcpy(&head.desired, &head.current, sizeof(struct HeadState));
	head.desired.transform = 0;

	print_head_desired(INFO, &head);

	assert_log(INFO, "    transform: none\n");
}

static void print_active__empty(void **state) {
	print_list(INFO, NULL);
}

static void print_active__many(void **state) {
	struct State *s = *state;

	s->head1->current.enabled = false;
	print_list(INFO, s->heads);

	char *expected_log = read_file("tst/info/print-list.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

static void print_adaptive_sync_fail__nulls(void **state) {
	print_adaptive_sync_fail(ERROR, NULL);
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
}

static void print_mode_fail__nulls(void **state) {

	print_mode_fail(WARNING, NULL, NULL);

	assert_log(WARNING, "\nChanges failed\n");
}

static void print_mode_fail__head(void **state) {
	struct Head *head = head_init();
	head->name = strdup("head0");
	head->model = strdup("model0");

	print_mode_fail(WARNING, head, NULL);

	assert_log(WARNING, "\nChanges failed\n  head0:\n    (no mode)\n");

	head_free(head);
}

static void print_heads_outstanding__many(void **state) {
	struct Pslist *heads = NULL;

	will_return_int(__wrap_log_get_threshold, DEBUG);

	struct Head *head_reapply = head_n("re");
	head_reapply->reapply_required = true;
	pslist_append(&heads, head_reapply);

	struct Head *head_mode = head_n("mo");
	head_mode->desired.mode = mode_init();
	pslist_append(&heads, head_mode);

	struct Head *head_disable = head_n("di");
	head_disable->current.enabled = true;
	head_disable->desired.enabled = false;
	pslist_append(&heads, head_disable);

	struct Head *head_vrr = head_n("vr");
	head_vrr->current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
	head_vrr->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	pslist_append(&heads, head_vrr);

	struct Head *head_enable = head_n("en");
	head_enable->current.enabled = false;
	head_enable->desired.enabled = true;
	pslist_append(&heads, head_enable);

	struct Head *head_scale = head_n("sc");
	head_scale->current.scale = 1;
	head_scale->desired.scale = 2;
	pslist_append(&heads, head_scale);

	struct Head *head_x = head_n("x");
	head_x->current.x = 1;
	head_x->desired.x = 2;
	pslist_append(&heads, head_x);

	struct Head *head_y = head_n("y");
	head_y->current.y = 1;
	head_y->desired.y = 2;
	pslist_append(&heads, head_y);

	struct Head *head_transform = head_n("tr");
	head_transform->current.transform = WL_OUTPUT_TRANSFORM_90;
	head_transform->desired.transform = WL_OUTPUT_TRANSFORM_180;
	pslist_append(&heads, head_transform);

	struct Head *head_all = head_n("a");
	head_all->reapply_required = true;
	head_all->desired.mode = mode_init();
	head_all->current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
	head_all->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	head_all->current.x = 1;
	head_all->desired.x = 2;
	head_all->current.enabled = false;
	head_all->desired.enabled = true;
	pslist_append(&heads, head_all);

	print_head_queue(DEBUG, "foo", IDLE, heads);

	assert_log(DEBUG, "foo IDLE queue re:reapply ; a:reapply ; mo:mode ; a:mode ; vr:vrr ; a:vrr ; di:disable en:enable sc:geometry x:geometry y:geometry tr:geometry a:enable a:geometry\n");

	pslist_free_vals(&heads, (fn_free)head_free);
}

static void print_heads_outstanding__none(void **state) {

	will_return_int(__wrap_log_get_threshold, DEBUG);

	print_head_queue(DEBUG, "foo", IDLE, NULL);

	assert_log(DEBUG, "foo IDLE queue\n");
}

static void print_heads_outstanding__below_threshold(void **state) {
	const struct State *s = *state;

	will_return_int(__wrap_log_get_threshold, WARNING);

	print_head_queue(DEBUG, "foo", IDLE, s->heads);
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

		TEST_BA(print_active__empty),
		TEST_BA(print_active__many),

		TEST_BA(print_adaptive_sync_fail__nulls),
		TEST_BA(print_adaptive_sync_fail__head),

		TEST_BA(print_mode_fail__nulls),
		TEST_BA(print_mode_fail__head),

		TEST_BA(print_heads_outstanding__many),
		TEST_BA(print_heads_outstanding__none),
		TEST_BA(print_heads_outstanding__below_threshold),
	};

	return RUN(tests);
}

