#include "tst.h"
#include "asserts.h"
#include "util.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "cfg.h"
#include "slist.h"
#include "log.h"

#include "info.h"

struct Head *head = NULL;

int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	head = calloc(1, sizeof(struct Head));

	struct Mode *mode_cur = mode_init(head, NULL, 100, 200, 30000, true);
	struct Mode *mode_des = mode_init(head, NULL, 400, 500, 60000, false);
	struct Mode *mode_failed = mode_init(head, NULL, 700, 800, 90000, false);

	slist_append(&head->modes, mode_cur);
	slist_append(&head->modes, mode_des);
	slist_append(&head->modes, mode_failed);
	slist_append(&head->modes_failed, mode_failed);

	head->name = strdup("name");
	head->description = strdup("description");
	head->width_mm = 1;
	head->height_mm = 2;
	head->make = strdup("make");
	head->model = strdup("model");
	head->serial_number = strdup("serial_number");

	head->current.mode = mode_cur;
	head->current.scale = 512;
	head->current.enabled = true;
	head->current.x = 700;
	head->current.y = 800;
	head->current.transform = WL_OUTPUT_TRANSFORM_180;
	head->current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	head->desired.mode = mode_des;
	head->desired.scale = 1024;
	head->desired.enabled = true;
	head->desired.x = 900;
	head->desired.y = 1000;
	head->desired.transform = WL_OUTPUT_TRANSFORM_90;
	head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	return 0;
}

int after_each(void **state) {
	head_free(head);

	head = NULL;

	assert_logs_empty();

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
	print_head(INFO, ARRIVED, head);

	char *expected_log = read_file("tst/info/print-head-arrived-all.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

void print_head_arrived__min(void **state) {
	head_free(head);
	head = calloc(1, sizeof(struct Head));

	print_head(INFO, ARRIVED, head);

	char *expected_log = read_file("tst/info/print-head-arrived-min.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

void print_head_departed(void **state) {
	print_head(INFO, DEPARTED, head);

	char *expected_log = read_file("tst/info/print-head-departed.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

void print_head_deltas__mode(void **state) {
	head->current.enabled = true;
	head->desired.enabled = true;

	print_head(INFO, DELTA, head);

	char *expected_log = read_file("tst/info/print-head-deltas-mode.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

void print_head_deltas__vrr(void **state) {
	head->current.enabled = true;
	head->desired.enabled = true;
	head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	head->desired.mode = head->current.mode;

	print_head(INFO, DELTA, head);

	char *expected_log = read_file("tst/info/print-head-deltas-vrr.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

void print_head_deltas__other(void **state) {
	head->current.enabled = false;
	head->desired.enabled = true;
	head->desired.mode = head->current.mode;

	print_head(INFO, DELTA, head);

	char *expected_log = read_file("tst/info/print-head-deltas-other.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

void print_head_deltas__disable(void **state) {
	head->current.enabled = true;
	head->desired.enabled = false;

	print_head(INFO, DELTA, head);

	char *expected_log = read_file("tst/info/print-head-deltas-disable.log");
	assert_log(INFO, expected_log);
	free(expected_log);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(print_cfg_commands__empty),
		TEST(print_cfg_commands__ok),

		TEST(print_head_arrived__all),
		TEST(print_head_arrived__min),
		TEST(print_head_departed),
		TEST(print_head_deltas__mode),
		TEST(print_head_deltas__vrr),
		TEST(print_head_deltas__other),
		TEST(print_head_deltas__disable),
	};

	return RUN(tests);
}

