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

int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	return 0;
}

int after_each(void **state) {
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

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(print_cfg_commands__empty),
		TEST(print_cfg_commands__ok),
	};

	return RUN(tests);
}

