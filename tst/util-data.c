#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "util-init.h"
#include "util-col.h"

#include "cfg.h"
#include "cfg/condition.h"
#include "cfg/disabled.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "log.h"
#include "pset.h"
#include "pslist.h"
#include "wlr-output-management-unstable-v1.h"

#include "util-data.h"

void log_cap_line_append(enum LogThreshold threshold, const char *line, struct Pslist **log_cap_lines) {
	struct LogCapLine *lcl = calloc(1, sizeof(struct LogCapLine));

	lcl->threshold = threshold;
	lcl->line = strdup(line);

	pslist_append(log_cap_lines, lcl);
}

// cfg-all.yaml
struct Cfg *cfg_all(void) {
	struct Cfg *cfg = cfg_default();

	cfg->arrange = COL;
	cfg->align = BOTTOM;
	cfg->scaling = OFF;
	cfg->auto_scale = OFF;
	cfg->log_threshold = ERROR;
	cfg->scale_round_strategy = UP;
	cfg->scale_round_to = 4;

	cfg->auto_scale_dpi = 48;
	cfg->auto_scale_min = 0.5f;
	cfg->auto_scale_max = 2.5f;

	free(cfg->callback_cmd);
	cfg->callback_cmd = strdup("cmd");
	cfg->laptop_display_prefix = strdup("ldp");
	cfg->laptop_lid_monitor = OFF;

	sset_add_many(cfg->order_name_desc,
			"one",
			"ONE",
			"!two",
			NULL);

	simap_put_many(cfg->scales,
			"three", 3000,
			"four", 4000,
			NULL);

	spmap_put_many(cfg->modes,
			"five", mode_whr(1920, 1080, 12340),
			"six", mode_whr(2560, 1440, -1),
			"seven", mode_whr_max(-1, -1, -1),
			NULL);

	sset_add_many(cfg->adaptive_sync_off,
			"ten",
			"ELEVEN",
			NULL);

	pset_add_many(cfg->disableds,
			disabled_nd("eight"),
			disabled_nd("EIGHT"),
			disabled_nd("nine"),
			NULL);

	struct Disabled *disabled = disabled_init();
	disabled->name_desc = strdup("twelve");

	struct Condition *cond = condition_init();
	sset_add_many(cond->plugged, "ONE", "TWO", NULL);
	pset_add(disabled->conditions, cond);

	cond = condition_init();
	sset_add_many(cond->unplugged, "THREE", NULL);
	pset_add(disabled->conditions, cond);

	cond = condition_init();
	cond->lid = LID_CLOSED;
	pset_add(disabled->conditions, cond);

	pset_add(cfg->disableds, disabled);

	simap_put_many(cfg->transforms,
			"twelve", WL_OUTPUT_TRANSFORM_FLIPPED,
			NULL);

	return cfg;
}

// ipc-responses-map.yaml and ipc-responses-seq.yaml
struct IpcOperation *ipc_response(void) {
	struct IpcRequest *ipc_request = calloc(1, sizeof(struct IpcRequest));
	ipc_request->log_threshold = WARNING;
	ipc_request->command = GET;

	struct IpcOperation *ipc_operation = calloc(1, sizeof(struct IpcOperation));
	ipc_operation->request = ipc_request;
	ipc_operation->done = true;
	ipc_operation->rc = 0;
	ipc_operation->send_state = true;

	g_cfg = cfg_all();

	g_lid = calloc(1, sizeof(struct Lid));
	g_lid->closed = true;
	g_lid->device_path = "/path/to/lid";

	log_cap_line_append(DEBUG, "dbg", &ipc_operation->log_cap_lines);
	log_cap_line_append(INFO, "inf", &ipc_operation->log_cap_lines);
	log_cap_line_append(WARNING, "war", &ipc_operation->log_cap_lines);

	ipc_operation_update_rc(ipc_operation);

	struct Head *head0 = head_init();

	head0->name = strdup("name");
	head0->description = strdup("desc");
	head0->width_mm = 1;
	head0->height_mm = 2;
	head0->make = strdup("make");
	head0->model = strdup("model");
	head0->serial_number = strdup("serial");
	head0->overrided_enabled = true;

	head0->current.scale = wl_fixed_from_double(4.0);
	head0->current.enabled = true;
	head0->current.x = 5;
	head0->current.y = 6;
	head0->current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	head0->current.transform = WL_OUTPUT_TRANSFORM_270;

	const struct Mode *mode_cur = mode_h_whr(head0, 10, 11, 12);
	head0->mode_preferred = mode_cur;
	head0->current.mode = mode_cur;
	pset_add(head0->modes, head0->current.mode);

	head0->desired.scale = wl_fixed_from_double(7.0);
	head0->desired.enabled = true;
	head0->desired.x = 8;
	head0->desired.y = 9;
	head0->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
	head0->desired.transform = WL_OUTPUT_TRANSFORM_FLIPPED;

	head0->desired.mode = mode_h_whr(head0, 13, 14, 15);
	pset_add(head0->modes, head0->desired.mode);

	pset_add(head0->modes_failed, mode_h_whr(head0, 16, 17, 18));

	pslist_append(&g_heads, head0);

	return ipc_operation;
}

