#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "util-init.h"
#include "util-col.h"

#include "cfg/cfg.h"
#include "cfg/condition.h"
#include "cfg/disabled.h"
#include "displ.h"
#include "enum.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "log.h"
#include "plist.h"
#include "ppmap.h"
#include "pset.h"
#include "simap.h"
#include "spmap.h"
#include "wlr-output-management-unstable-v1.h"

#include "data.h"

// head ppmap keys
void *H0 = "H0";
void *H1 = "H1";
void *H2 = "H2";
void *H3 = "H3";
void *H4 = "H4";
void *H5 = "H5";
void *H6 = "H6";
void *H7 = "H7";
void *H8 = "H8";
void *H9 = "H9";

// mode ppmap keys
void *MC = "MC";
void *MD = "MD";
void *MP = "MP";
void *MF = "MF";
void *MR = "MR";

void *M0 = "M0";
void *M1 = "M1";
void *M2 = "M2";
void *M3 = "M3";
void *M4 = "M4";
void *M5 = "M5";
void *M6 = "M6";
void *M7 = "M7";
void *M8 = "M8";
void *M9 = "M9";
void *M10 = "M10";

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

	simap_put(cfg->scales, "three", 3000);
	simap_put(cfg->scales, "four", 4000);

	spmap_put_many(cfg->modes,
			"five", mode_whr(1920, 1080, 12340),
			"six", mode_whr(2560, 1440, -1),
			"seven", mode_whr_max(-1, -1, -1),
			NULL);

	sset_add_many(cfg->adaptive_sync_off,
			"ten",
			"ELEVEN",
			NULL);

	spmap_put_many(cfg->disableds,
			"eight", cfg_disabled_init(),
			"EIGHT", cfg_disabled_init(),
			"nine",  cfg_disabled_init(),
			NULL);

	const struct CfgDisabled *disabled = cfg_disabled_init();

	struct CfgCondition *cond = cfg_condition_init();
	sset_add_many(cond->plugged, "ONE", "TWO", NULL);
	pset_add(disabled->conditions, cond);

	cond = cfg_condition_init();
	sset_add_many(cond->unplugged, "THREE", NULL);
	pset_add(disabled->conditions, cond);

	cond = cfg_condition_init();
	cond->lid = LID_CLOSED;
	pset_add(disabled->conditions, cond);

	spmap_put(cfg->disableds, "twelve", disabled);

	simap_put(cfg->transforms, "twelve", (size_t)WL_OUTPUT_TRANSFORM_FLIPPED);

	return cfg;
}

// ipc-responses-map.yaml and ipc-responses-seq.yaml
struct IpcOperation *ipc_response(void) {
	struct IpcRequest *ipc_request = ipc_request_init(GET);
	ipc_request->log_threshold = WARNING;

	struct IpcOperation *ipc_operation = ipc_operation_init();
	ipc_operation->request = ipc_request;
	ipc_operation->done = true;
	ipc_operation->rc = 0;
	ipc_operation->send_state = true;

	g_cfg = cfg_all();

	g_lid = calloc(1, sizeof(struct Lid));
	g_lid->closed = true;
	g_lid->device_path = "/path/to/lid";

	plist_append(ipc_operation->log_cap_lines, log_cap_line_init(DEBUG, "dbg"));
	plist_append(ipc_operation->log_cap_lines, log_cap_line_init(INFO, "inf"));
	plist_append(ipc_operation->log_cap_lines, log_cap_line_init(WARNING, "war"));

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

	head0->cur.scale = wl_fixed_from_double(4.0);
	head0->cur.enabled = true;
	head0->cur.x = 5;
	head0->cur.y = 6;
	head0->cur.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	head0->cur.transform = WL_OUTPUT_TRANSFORM_270;

	ppmap_put(head0->modes, MC, mode_whr(10, 11, 12));
	head0->zmode_pref = MC;
	head0->cur.zmode = MC;
	head0->des.zmode = MD;

	head0->des.scale = wl_fixed_from_double(7.0);
	head0->des.enabled = true;
	head0->des.x = 8;
	head0->des.y = 9;
	head0->des.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
	head0->des.transform = WL_OUTPUT_TRANSFORM_FLIPPED;

	ppmap_put(head0->modes, MD, mode_whr(13, 14, 15));
	head0->des.zmode = MD;

	ppmap_put(head0->modes_failed, M0, mode_whr(16, 17, 18));

	ppmap_put(g_displ->heads, H0, head0);

	return ipc_operation;
}

