#include <stdbool.h>
#include <stdlib.h>
#include <wayland-util.h>
#include <yaml.h>

#include "yaml/unmarshal-types-ipc.h"

#include "enum.h"
#include "fn.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "log.h"
#include "mode.h"
#include "plist.h"
#include "ppmap.h"
#include "spmap.h"
#include "wlr-output-management-unstable-v1.h"
#include "yaml/unmarshal-primitives.h"
#include "yaml/unmarshal-types-cfg.h"
#include "yaml/unmarshal.h"

void *yaml_root_to_ipc_request(struct UC *c, const yaml_node_t *root) {
	c->t = ERROR;
	yaml_unmarshal_log_ctx_top(c, "document");

	const struct SPmap *m;
	if (!root || !(m = yaml_map_to_spmap(c, root)))
		return NULL;

	struct IpcRequest *ipc_request = ipc_request_init(0);

	yaml_unmarshal_log_ctx_top(c, "OP");
	const yaml_node_t *op = spmap_get(m, "OP");
	if (!yaml_check_mandatory(c, op) || !(ipc_request->command = yaml_scalar_to_enum(c, op, ipc_command_val, ipc_command_names)))
		goto err;

	// log warnings for remainder
	c->t = WARNING;

	yaml_unmarshal_log_ctx_top(c, "LOG_THRESHOLD");
	const yaml_node_t *log_threshold = spmap_get(m, "LOG_THRESHOLD");
	if (log_threshold)
		ipc_request->log_threshold = yaml_scalar_to_enum(c, log_threshold, log_threshold_val, log_threshold_names);

	yaml_unmarshal_log_ctx_top(c, "CFG");
	const yaml_node_t *cfg = spmap_get(m, "CFG");
	if (cfg)
		ipc_request->cfg = yaml_map_to_cfg(c, cfg);

	goto end;

err:
	ipc_request_free(ipc_request);
	ipc_request = NULL;

end:
	spmap_free(m);

	return ipc_request;
}

void *yaml_root_to_ipc_response_plist(struct UC *c, const yaml_node_t *root) {
	if (!root)
		return NULL;

	const struct Plist *ipc_responses = ipc_response_plist_init();

	// fail on bad type
	c->t = ERROR;
	yaml_unmarshal_log_ctx_top(c, "document");
	if (!yaml_check_node_type(c, root, YAML_SEQUENCE_NODE, YAML_MAPPING_NODE)) {
		goto err;
	}

	if (root->type == YAML_SEQUENCE_NODE) {
		for (const yaml_node_item_t *item = root->data.sequence.items.start; item < root->data.sequence.items.top; item ++) {
			yaml_map_into_ipc_responses(c, ipc_responses, yaml_document_get_node(&c->d, *item));
		}
	} else {
		yaml_map_into_ipc_responses(c, ipc_responses, root);
	}

	if (plist_size(ipc_responses) == 0) {
		goto err;
	}

	return (void*)ipc_responses;

err:
	plist_free_vals(ipc_responses);
	return NULL;
}

void yaml_map_into_ipc_responses(struct UC *c, const struct Plist* const ipc_responses, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!ipc_responses || !(m = yaml_map_to_spmap(c, map)))
		return;

	// log exceptions and fail for required fields
	c->t = ERROR;

	struct IpcResponse *ipc_response = ipc_response_init();

	yaml_unmarshal_log_ctx_top(c, "DONE");
	const yaml_node_t *done = spmap_get(m, "DONE");
	if (!yaml_check_mandatory(c, done) || !yaml_scalar_to_boolean(c, &ipc_response->status.done, done))
		goto err;

	yaml_unmarshal_log_ctx_top(c, "RC");
	const yaml_node_t *rc = spmap_get(m, "RC");
	if (!yaml_check_mandatory(c, rc) || !yaml_scalar_to_int(c, &ipc_response->status.rc, rc))
		goto err;

	// suppress validation failures for remainder
	c->t = 0;

	yaml_unmarshal_log_ctx_top(c, "CFG");
	const yaml_node_t *cfg = spmap_get(m, "CFG");
	if (cfg)
		ipc_response->cfg = yaml_map_to_cfg(c, cfg);

	yaml_unmarshal_log_ctx_top(c, "STATE");
	const yaml_node_t *state = spmap_get(m, "STATE");
	if (state) {
		const struct SPmap *ms = yaml_map_to_spmap(c, state);
		if (ms) {

			ipc_response->lid =	yaml_map_to_lid(c, spmap_get(ms, "LID"));

			yaml_seq_into_col(c, spmap_get(ms, "HEADS"), ipc_response->heads, (fn_yaml_node_into_col)yaml_map_into_heads);

			spmap_free(ms);
		}
	}

	yaml_unmarshal_log_ctx_top(c, "MESSAGES");
	const yaml_node_t *messages = spmap_get(m, "MESSAGES");
	if (messages) {
		yaml_seq_into_col(c, messages, ipc_response->log_cap_lines, (fn_yaml_node_into_col)yaml_map_into_log_cap_lines);
	}

	plist_append(ipc_responses, ipc_response);

	goto end;

err:
	ipc_response_free(ipc_response);
	ipc_response = NULL;

end:
	spmap_free(m);
}

struct Lid *yaml_map_to_lid(struct UC *c, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!(m = yaml_map_to_spmap(c, map)))
		return NULL;

	struct Lid *lid = (struct Lid*)calloc(1, sizeof(struct Lid));

	lid->device_path = yaml_scalar_to_string(c, spmap_get(m, "DEVICE_PATH"));
	yaml_scalar_to_boolean(c, &lid->closed, spmap_get(m, "CLOSED"));

	spmap_free(m);

	return lid;
}

struct Mode *yaml_map_to_head_mode(struct UC *c, const yaml_node_t *map) {
	const struct SPmap *m = yaml_map_to_spmap(c, map);
	if (!m)
		return NULL;

	struct Mode *mode = mode_init();

	yaml_scalar_to_int(c, &mode->width, spmap_get(m, "WIDTH"));
	yaml_scalar_to_int(c, &mode->height, spmap_get(m, "HEIGHT"));
	yaml_scalar_to_int(c, &mode->refresh_mhz, spmap_get(m, "REFRESH_MHZ"));

	spmap_free(m);

	return mode;
}

void yaml_map_into_head_modes(struct UC *c, const struct PPmap* const modes, const yaml_node_t *map) {
	if (!modes)
		return;

	const struct Mode *mode = yaml_map_to_head_mode(c, map);
	ppmap_put(modes, mode, mode);
}

void yaml_map_into_heads(struct UC *c, const struct Plist* const heads, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!heads || !(m = yaml_map_to_spmap(c, map)))
		return;

	struct Head *head = head_init();

	// use mode equals so that we can map preferred/current/desired to the lists
	ppmap_free(head->modes);
	head->modes = mode_ppmap_equal_init();
	ppmap_free(head->modes_failed);
	head->modes_failed = mode_ppmap_equal_init();

	head->name           = yaml_scalar_to_string(c, spmap_get(m, "NAME"));
	head->description    = yaml_scalar_to_string(c, spmap_get(m, "DESCRIPTION"));
	head->make           = yaml_scalar_to_string(c, spmap_get(m, "MAKE"));
	head->model          = yaml_scalar_to_string(c, spmap_get(m, "MODEL"));
	head->serial_number  = yaml_scalar_to_string(c, spmap_get(m, "SERIAL_NUMBER"));
	yaml_scalar_to_int      (c, &head->width_mm,    spmap_get(m, "WIDTH_MM"));
	yaml_scalar_to_int      (c, &head->height_mm,   spmap_get(m, "HEIGHT_MM"));

	yaml_seq_into_col(c, spmap_get(m, "MODES"),        head->modes,        (fn_yaml_node_into_col)yaml_map_into_head_modes);
	yaml_seq_into_col(c, spmap_get(m, "MODES_FAILED"), head->modes_failed, (fn_yaml_node_into_col)yaml_map_into_head_modes);

	// find MODE_PREFERRED in MODES/MODES_FAILED and assign the key
	struct Mode *mode_pref = yaml_map_to_head_mode(c, spmap_get(m, "MODE_PREFERRED"));
	if (mode_pref) {
		struct PPmapFilter f = { .val_data = (fn_pred_pp)mode_equal, .data = mode_pref, };
		head->zmode_pref = ppmap_find(head->modes, f).key;
		if (!head->zmode_pref) {
			head->zmode_pref = ppmap_find(head->modes_failed, f).key;
		}
		mode_free(mode_pref);
	}

	yaml_map_into_head_state(c, &head->cur, head, spmap_get(m, "CURRENT"));
	yaml_map_into_head_state(c, &head->des, head, spmap_get(m, "DESIRED"));

	const struct SPmap *mo = yaml_map_to_spmap(c, spmap_get(m, "OVERRIDES"));
	if (mo) {
		bool disabled;
		if (yaml_scalar_to_boolean(c, &disabled, spmap_get(mo, "DISABLED"))) {
			head->overrided_enabled = disabled ? OverrideFalse : OverrideTrue;
		}
	}

	plist_append(heads, head);

	spmap_free(m);
	spmap_free(mo);
}

void yaml_map_into_head_state(struct UC *c, struct HeadState *head_state, const struct Head * const head, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!head_state || !(m = yaml_map_to_spmap(c, map)))
		return;

	yaml_scalar_to_boolean(c, &head_state->enabled, spmap_get(m, "ENABLED"));

	yaml_scalar_to_int(c, &head_state->x, spmap_get(m, "X"));
	yaml_scalar_to_int(c, &head_state->y, spmap_get(m, "Y"));

	head_state->transform = yaml_scalar_to_enum(c, spmap_get(m, "TRANSFORM"), transform_val, NULL);

	float scale;
	if (yaml_scalar_to_float(c, &scale, spmap_get(m, "SCALE")))
		head_state->scale = wl_fixed_from_double(scale);

	bool vrr = false;
	if (yaml_scalar_to_boolean(c, &vrr, spmap_get(m, "VRR")))
		head_state->adaptive_sync = vrr ? ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED : ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	// find MODE in MODES/MODES_FAILED and assign the key
	struct Mode *mode = yaml_map_to_head_mode(c, spmap_get(m, "MODE"));
	if (mode) {
		head_state->zmode = ppmap_first_key(head->modes, mode);
		if (!head_state->zmode) {
			head_state->zmode = ppmap_first_key(head->modes_failed, mode);
		}
		mode_free(mode);
	}

	spmap_free(m);

	return;
}

void yaml_map_into_log_cap_lines(struct UC *c, const struct Plist* const log_cap_lines, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!log_cap_lines || !(m = yaml_map_to_spmap(c, map)))
		return;

	// unmarshal many pairs even though schema specifies exactly one
	for (const struct SPmapIt *it = spmap_it(m); it; it = spmap_it_next(it)) {

		enum LogThreshold threshold = log_threshold_val(it->key);
		char *line = yaml_scalar_to_string(c, it->val);

		if (threshold && line)
			plist_append(log_cap_lines, log_cap_line_init(threshold, line));

		free(line);
	}

	spmap_free(m);
}

