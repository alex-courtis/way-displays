#include "tst.h"
#include "asserts.h"
#include "util.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "cfg.h"
#include "displ.h"
#include "head.h"
#include "log.h"
#include "mode.h"
#include "slist.h"
#include "wlr-output-management-unstable-v1.h"

#include "info.h"

struct State {
	struct Head *head1;
	struct Head *head2;
	struct SList *heads;
};

int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	struct State *s = calloc(1, sizeof(struct State));

	s->head1 = calloc(1, sizeof(struct Head));

	struct Mode *mode_cur = mode_init(s->head1, NULL, 100, 200, 30000, true);
	struct Mode *mode_des = mode_init(s->head1, NULL, 400, 500, 60000, false);
	struct Mode *mode_failed = mode_init(s->head1, NULL, 700, 800, 90000, false);

	slist_append(&s->head1->modes, mode_cur);
	slist_append(&s->head1->modes, mode_des);
	slist_append(&s->head1->modes, mode_failed);
	slist_append(&s->head1->modes_failed, mode_failed);

	s->head1->name = strdup("name1");
	s->head1->description = strdup("description1");
	s->head1->width_mm = 1;
	s->head1->height_mm = 2;
	s->head1->make = strdup("make1");
	s->head1->model = strdup("model1");
	s->head1->serial_number = strdup("serial_number1");

	s->head1->current.mode = mode_cur;
	s->head1->current.scale = 512;
	s->head1->current.enabled = true;
	s->head1->current.x = 700;
	s->head1->current.y = 800;
	s->head1->current.transform = WL_OUTPUT_TRANSFORM_180;
	s->head1->current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	s->head1->desired.mode = mode_des;
	s->head1->desired.scale = 1024;
	s->head1->desired.enabled = true;
	s->head1->desired.x = 900;
	s->head1->desired.y = 1000;
	s->head1->desired.transform = WL_OUTPUT_TRANSFORM_90;
	s->head1->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	slist_append(&s->heads, s->head1);


	s->head2 = calloc(1, sizeof(struct Head));

	mode_cur = mode_init(s->head2, NULL, 1100, 1200, 130000, true);
	mode_des = mode_init(s->head2, NULL, 1400, 1500, 160000, false);
	mode_failed = mode_init(s->head2, NULL, 1700, 1800, 190000, false);

	slist_append(&s->head2->modes, mode_cur);
	slist_append(&s->head2->modes, mode_des);
	slist_append(&s->head2->modes, mode_failed);
	slist_append(&s->head2->modes_failed, mode_failed);

	s->head2->name = strdup("name2");
	s->head2->width_mm = 3;
	s->head2->height_mm = 4;
	s->head2->make = strdup("make2");
	s->head2->model = strdup("model2");
	s->head2->serial_number = strdup("serial_number2");

	s->head2->current.mode = mode_cur;
	s->head2->current.scale = 2048;
	s->head2->current.enabled = true;
	s->head2->current.x = 1700;
	s->head2->current.y = 1800;
	s->head2->current.transform = WL_OUTPUT_TRANSFORM_270;
	s->head2->current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	s->head2->desired.mode = mode_des;
	s->head2->desired.scale = 4096;
	s->head2->desired.enabled = true;
	s->head2->desired.x = 1900;
	s->head2->desired.y = 11000;
	s->head2->desired.transform = WL_OUTPUT_TRANSFORM_NORMAL;
	s->head2->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	slist_append(&s->heads, s->head2);

	*state = s;
	return 0;
}

int after_each(void **state) {
	assert_logs_empty();

	struct State *s = *state;

	slist_free_vals(&s->heads, head_free);

	free(s);

	return 0;
}

void print_cfg_commands__empty(void **state) {
	struct Cfg *cfg = cfg_init();

	print_cfg_commands(INFO, cfg);

	cfg_free(cfg);
}

void print_cfg_commands__ok(void **state) {
	struct Cfg *cfg = cfg_default();

	cfg->arrange = COL;
	cfg->align = RIGHT;

	slist_append(&cfg->order_name_desc, strdup("one"));
	slist_append(&cfg->order_name_desc, strdup("two"));
	slist_append(&cfg->order_name_desc, strdup("three"));

	cfg->scaling = OFF;

	cfg->auto_scale = OFF;

	slist_append(&cfg->user_scales, cfg_user_scale_init("one", 1));
	slist_append(&cfg->user_scales, cfg_user_scale_init("two", 2.3456));

	slist_append(&cfg->user_modes, cfg_user_mode_init("all", false, 1, 2, 12340, false));
	slist_append(&cfg->user_modes, cfg_user_mode_init("res", false, 4, 5, -1, false));
	slist_append(&cfg->user_modes, cfg_user_mode_init("max", true, 7, 8, 9, false));

	slist_append(&cfg->user_transforms, cfg_user_transform_init("seven", WL_OUTPUT_TRANSFORM_FLIPPED_90));

	slist_append(&cfg->disabled_name_desc, strdup("three"));
	slist_append(&cfg->disabled_name_desc, strdup("four"));

	slist_append(&cfg->adaptive_sync_off_name_desc, strdup("five"));
	slist_append(&cfg->adaptive_sync_off_name_desc, strdup("six"));

	print_cfg_commands(INFO, cfg);

	char *expected_log = read_file("tst/info/print-cfg-commands-ok.log");
	assert_log(INFO, expected_log);

	cfg_free(cfg);
	free(expected_log);
}

void print_head_arrived__all(void **state) {
	struct State *s = *state;

	print_head(INFO, ARRIVED, s->head1);

	char *expected_log = read_file("tst/info/print-head-arrived-all.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

void print_head_arrived__min(void **state) {
	struct Head *head = calloc(1, sizeof(struct Head));

	print_head(INFO, ARRIVED, head);

	char *expected_log = read_file("tst/info/print-head-arrived-min.log");
	assert_log(INFO, expected_log);
	free(expected_log);

	head_free(head);
}

void print_head_departed__ok(void **state) {
	struct State *s = *state;

	print_head(INFO, DEPARTED, s->head1);

	char *expected_log = read_file("tst/info/print-head-departed-ok.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

void print_head_deltas__mode(void **state) {
	struct State *s = *state;

	print_head(INFO, DELTA, s->head1);

	char *expected_log = read_file("tst/info/print-head-deltas-mode.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

void print_head_deltas__vrr(void **state) {
	struct State *s = *state;

	s->head1->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	s->head1->desired.mode = s->head1->current.mode;

	print_head(INFO, DELTA, s->head1);

	char *expected_log = read_file("tst/info/print-head-deltas-vrr.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

void print_head_deltas__other(void **state) {
	struct State *s = *state;

	s->head1->desired.mode = s->head1->current.mode;

	print_head(INFO, DELTA, s->head1);

	char *expected_log = read_file("tst/info/print-head-deltas-other.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

void print_head_deltas__disable(void **state) {
	struct State *s = *state;

	s->head1->desired.enabled = false;

	print_head(INFO, DELTA, s->head1);

	char *expected_log = read_file("tst/info/print-head-deltas-disable.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

void print_head_deltas__enable(void **state) {
	struct State *s = *state;

	s->head1->current.enabled = false;

	print_head(INFO, DELTA, s->head1);

	char *expected_log = read_file("tst/info/print-head-deltas-enable.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

void delta_human_mode__to_no(void **state) {
	struct State *s = *state;

	s->head1->desired.mode = NULL;

	char *deltas = delta_human_mode(SUCCEEDED, s->head1);

	assert_string_equal(deltas, ""
			"description1\n"
			"  100x200@30Hz -> (no mode)"
			);

	slist_free(&heads);

	free(deltas);
}

void delta_human_mode__from_no(void **state) {
	struct State *s = *state;

	s->head2->current.mode = NULL;

	char *deltas = delta_human_mode(SUCCEEDED, s->head2);

	assert_string_equal(deltas, ""
			"name2\n"
			"  (no mode) -> 1400x1500@160Hz"
			);

	slist_free(&heads);

	free(deltas);
}

void delta_human_adaptive_sync__on(void **state) {
	struct State *s = *state;

	s->head1->current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
	s->head1->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;

	char *deltas = delta_human_adaptive_sync(SUCCEEDED, s->head1);

	assert_string_equal(deltas, ""
			"description1\n"
			"  VRR on"
			);

	slist_free(&heads);

	free(deltas);
}

void delta_human_adaptive_sync__off(void **state) {
	struct State *s = *state;

	s->head2->current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	s->head2->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	char *deltas = delta_human_adaptive_sync(SUCCEEDED, s->head2);

	assert_string_equal(deltas, ""
			"name2\n"
			"  VRR off"
			);

	slist_free(&heads);

	free(deltas);
}

void delta_human__all(void **state) {
	struct State *s = *state;

	char *deltas = delta_human(SUCCEEDED, s->heads);

	assert_string_equal(deltas, ""
			"description1\n"
			"  scale:     2.000 -> 4.000\n"
			"  transform: 180 -> 90\n"
			"  position:  700,800 -> 900,1000\n"
			"name2\n"
			"  scale:     8.000 -> 16.000\n"
			"  transform: 270 -> none\n"
			"  position:  1700,1800 -> 1900,11000"
			);

	slist_free(&heads);

	free(deltas);
}

void delta_human__enabled(void **state) {
	struct State *s = *state;

	s->head1->current.enabled = false;
	s->head1->desired.enabled = true;

	s->head2->current.enabled = false;
	s->head2->desired.enabled = true;

	char *deltas = delta_human(SUCCEEDED, s->heads);

	assert_string_equal(deltas, ""
			"description1  enabled\n"
			"name2  enabled"
			);

	slist_free(&heads);

	free(deltas);
}

void delta_human__disabled(void **state) {
	struct State *s = *state;

	s->head1->current.enabled = true;
	s->head1->desired.enabled = false;

	s->head2->current.enabled = true;
	s->head2->desired.enabled = false;

	char *deltas = delta_human(SUCCEEDED, s->heads);

	assert_string_equal(deltas, ""
			"description1  disabled\n"
			"name2  disabled"
			);

	slist_free(&heads);

	free(deltas);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(print_cfg_commands__empty),
		TEST(print_cfg_commands__ok),

		TEST(print_head_arrived__all),
		TEST(print_head_arrived__min),
		TEST(print_head_departed__ok),

		TEST(print_head_deltas__mode),
		TEST(print_head_deltas__vrr),
		TEST(print_head_deltas__other),
		TEST(print_head_deltas__disable),
		TEST(print_head_deltas__enable),

		TEST(delta_human_mode__to_no),
		TEST(delta_human_mode__from_no),

		TEST(delta_human_adaptive_sync__on),
		TEST(delta_human_adaptive_sync__off),

		TEST(delta_human__all),
		TEST(delta_human__enabled),
		TEST(delta_human__disabled),
	};

	return RUN(tests);
}

