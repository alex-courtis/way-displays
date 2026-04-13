#include "tst.h"
#include "asserts.h"
#include "util.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "cfg.h"
#include "conditions.h"
#include "global.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "slist.h"
#include "log.h"
#include "mode.h"
#include "wlr-output-management-unstable-v1.h"

#include "marshalling.h"

void lcl(enum LogThreshold threshold, char *line, struct SList **log_cap_lines) {
	struct LogCapLine *lcl = calloc(1, sizeof(struct LogCapLine));

	lcl->threshold = threshold;
	lcl->line = strdup(line);

	slist_append(log_cap_lines, lcl);
}

int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	logs_clear();

	return 0;
}

int after_each(void **state) {
	cfg_free(cfg);
	cfg = NULL;
	free(lid);
	lid = NULL;
	return 0;
}


// cfg-all.yaml
struct Cfg *cfg_all(void) {
	struct Cfg *cfg = cfg_default();

	cfg->arrange = COL;
	cfg->align = BOTTOM;
	cfg->scaling = OFF;
	cfg->auto_scale = OFF;
	cfg->log_threshold = ERROR;

	cfg->auto_scale_min = 0.5f;
	cfg->auto_scale_max = 2.5f;

	free(cfg->callback_cmd);
	cfg->callback_cmd = strdup("cmd");
	cfg->laptop_display_prefix = strdup("ldp");

	slist_append(&cfg->order_name_desc, strdup("one"));
	slist_append(&cfg->order_name_desc, strdup("ONE"));
	slist_append(&cfg->order_name_desc, strdup("!two"));
	//
	// slist_append(&cfg->user_scales, cfg_user_scale_init("three", 3));
	// slist_append(&cfg->user_scales, cfg_user_scale_init("four", 4));
	//
	// slist_append(&cfg->user_modes, cfg_user_mode_init("five", false, 1920, 1080, 12340, false));
	// slist_append(&cfg->user_modes, cfg_user_mode_init("six", false, 2560, 1440, -1, false));
	// slist_append(&cfg->user_modes, cfg_user_mode_init("seven", true, -1, -1, -1, false));

	slist_append(&cfg->adaptive_sync_off_name_desc, strdup("ten"));
	slist_append(&cfg->adaptive_sync_off_name_desc, strdup("ELEVEN"));
	//
	// slist_append(&cfg->disabled, cfg_disabled_always("eight"));
	// slist_append(&cfg->disabled, cfg_disabled_always("EIGHT"));
	// slist_append(&cfg->disabled, cfg_disabled_always("nine"));
	//
	// struct Disabled *disabled = calloc(1, sizeof(struct Disabled));
	// disabled->name_desc = strdup("twelve");
	// struct Condition *cond = calloc(1, sizeof(struct Condition));
	// slist_append(&cond->plugged, strdup("ONE"));
	// slist_append(&disabled->conditions, cond);
	//
	// slist_append(&cfg->disabled, disabled);
	//
	// slist_append(&cfg->user_transforms, cfg_user_transform_init("twelve", WL_OUTPUT_TRANSFORM_FLIPPED));

	return cfg;
}

void unmarshal_cfg_from_file__ok_2(void **state) {

	struct Cfg *read = cfg_default();
	read->file_path = strdup("tst/marshalling/cfg-all.yaml");

	assert_true(unmarshal_cfg_from_file_2(read));

	struct Cfg *expected = cfg_all();

	assert_cfg_equal(read, expected);

	cfg_free(read);
	cfg_free(expected);

	// assert_logs_empty();
}

int main(void) {
	// unmarshal_cfg_from_file__ok_2(NULL);

	const struct CMUnitTest tests[] = {
		TEST(unmarshal_cfg_from_file__ok_2),
	};

	return RUN(tests);
}

