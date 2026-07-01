#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <wayland-util.h>

#include "info.h"

#include "cfg.h"
#include "cfg/disabled.h"
#include "cfg/user-mode.h"
#include "convert.h"
#include "fn.h"
#include "head.h"
#include "imap.h"
#include "lid.h"
#include "log.h"
#include "mode.h"
#include "output.h"
#include "process.h"
#include "pset.h"
#include "slist.h"
#include "smap.h"
#include "smapi.h"
#include "smaps.h"
#include "sset.h"
#include "str.h"
#include "wlr-output-management-unstable-v1.h"

char *info_user_mode_string(const struct UserMode * const user_mode) {
	if (!user_mode)
		return NULL;

	if (user_mode->max) {
		return sprintf_alloc("MAX");
	} else if (user_mode->refresh_mhz != -1) {
		return sprintf_alloc("%dx%d@%sHz",
				user_mode->width,
				user_mode->height,
				mhz_to_hz_str(user_mode->refresh_mhz)
				);
	} else {
		return sprintf_alloc("%dx%d",
				user_mode->width,
				user_mode->height
				);
	}
}

char *info_wlr_mode_string(const struct WlrMode * const wlr_mode) {
	if (!wlr_mode)
		return NULL;

	return sprintf_alloc("%dx%d@%dHz (%d,%03dmHz)%s",
			wlr_mode->width,
			wlr_mode->height,
			mhz_to_hz_rounded(wlr_mode->refresh_mhz),
			wlr_mode->refresh_mhz / 1000,
			wlr_mode->refresh_mhz % 1000,
			wlr_mode->preferred ? " (preferred)" : ""
			);
}

static void print_user_mode(const enum LogThreshold t, const char * name_desc, const struct UserMode * const user_mode, const bool del) {
	if (!user_mode)
		return;

	if (del) {
		log_(t, "    %s", name_desc);
	} else {
		char *um_str = info_user_mode_string(user_mode);
		log_(t, "    %s: %s", name_desc, um_str);
		free(um_str);
	}
}

static void print_wlr_mode(const enum LogThreshold t, const struct WlrMode * const wlr_mode) {

	if (wlr_mode) {
		char *mode_str = info_wlr_mode_string(wlr_mode);
		log_(t, "    mode:      %s", mode_str);
		free(mode_str);
	} else {
		log_(t, "    (no mode)");
	}
}

static void print_modes_failed(const enum LogThreshold t, const struct Head * const head) {
	if (!head)
		return;

	if (head->wlr_modes_failed) {
		log_(t, "  failed:");
		for (const struct SList *i = head->wlr_modes_failed; i; i = i->nex) {
			print_wlr_mode(t, i->val);
		}
	}
}

static void print_disabled(const enum LogThreshold t, const struct Disabled * const disabled) {
	if (!disabled) return;

	if (pset_size(disabled->conditions) > 0) {
		log_(t, "    %s (conditionally)", disabled->name_desc);
	} else {
		log_(t, "    %s", disabled->name_desc);
	}
}

static void print_modes_res_refresh(const enum LogThreshold t, const struct Head * const head) {
	if (!head)
		return;

	struct SList *mrrs = modes_res_refresh(head->wlr_modes);
	const struct WlrMode *wlr_mode_preferred = head_preferred_wlr_mode(head);

	struct ModesResRefresh *mrr = NULL;
	const struct WlrMode *wlr_mode = NULL;

	for (struct SList *i = mrrs; i; i = i->nex) {
		mrr = i->val;

		char *msg = sprintf_alloc("    mode:     %5d x%5d @%4d Hz ", mrr->width, mrr->height, mhz_to_hz_rounded(mrr->refresh_mhz));

		for (const struct SList *j = mrr->wlr_modes; j; j = j->nex) {
			wlr_mode = j->val;
			msg = sprintf_append(msg, "%4d,%03d mHz", wlr_mode->refresh_mhz / 1000, wlr_mode->refresh_mhz % 1000);
			if (wlr_mode == wlr_mode_preferred) {
				msg = sprintf_append(msg, " (preferred)");
			}
		}
		log_(t,"%s", msg);
		free(msg);
	}

	slist_free_vals(&mrrs, (fn_free)mode_res_refresh_free);
}

void print_cfg(const enum LogThreshold t, const struct Cfg * const cfg, const bool del) {
	if (!cfg)
		return;

	if (cfg->arrange && cfg->align) {
		log_(t, "  Arrange in a %s aligned at the %s", arrange_name(cfg->arrange), align_name(cfg->align));
	} else if (cfg->arrange) {
		log_(t, "  Arrange in a %s", arrange_name(cfg->arrange));
	} else if (cfg->align) {
		log_(t, "  Align at the %s", align_name(cfg->align));
	}

	if (sset_size(cfg->order_name_desc) > 0) {
		log_(t, "  Order:");
		for (const struct SSetIt *it = sset_it(cfg->order_name_desc); it; it = sset_it_next(it)) {
			log_(t, "    %s", it->val);
		}
	}

	if (cfg->scaling) {
		log_(t, "  Scaling: %s", on_off_name(cfg->scaling));
	}

	if (cfg->auto_scale) {
		if (cfg->auto_scale_max > 0) {
			log_(t, "  Auto scale: %s (dpi: %d, min: %0.3f, max: %0.3f)",
					on_off_name(cfg->auto_scale), cfg->auto_scale_dpi, cfg->auto_scale_min, cfg->auto_scale_max);
		} else {
			log_(t, "  Auto scale: %s (dpi: %d, min: %0.3f)",
					on_off_name(cfg->auto_scale), cfg->auto_scale_dpi, cfg->auto_scale_min);
		}
	}

	if (cfg->scale_round_to && cfg->scale_round_strategy) {
		log_(t, "  Round scales to: %s, %s", scale_round_to_name(cfg->scale_round_to), scale_round_strategy_name(cfg->scale_round_strategy));
	}

	if (smapi_size(cfg->scales) > 0) {
		log_(t, "  Scale:");
		for (const struct SMapIIt *it = smapi_it(cfg->scales); it; it = smapi_it_next(it)) {
			if (del) {
				log_(t, "    %s", it->key);
			} else {
				log_(t, "    %s: %.3f", it->key, (double)it->val/1000);
			}
		}
	}

	if (smap_size(cfg->user_modes) > 0) {
		log_(t, "  Mode:");
		for (const struct SMapIt *it = smap_it(cfg->user_modes); it; it = smap_it_next(it)) {
			print_user_mode(t, it->key, it->val, del);
		}
	}

	if (smapi_size(cfg->transforms) > 0) {
		log_(t, "  Transform:");
		for (const struct SMapIIt *it = smapi_it(cfg->transforms); it; it = smapi_it_next(it)) {
			if (del) {
				log_(t, "    %s", it->key);
			} else {
				log_(t, "    %s: %s", it->key, transform_name(it->val));
			}
		}
	}

	if (sset_size(cfg->max_preferred_refresh) > 0) {
		log_(t, "  Max preferred refresh:");
		for (const struct SSetIt *it = sset_it(cfg->max_preferred_refresh); it; it = sset_it_next(it)) {
			log_(t, "    %s", it->val);
		}
	}

	if (pset_size(cfg->disableds) > 0) {
		log_(t, "  Disabled:");
		for (const struct PSetIt *it = pset_it(cfg->disableds); it; it = pset_it_next(it)) {
			print_disabled(t, it->val);
		}
	}


	if (cfg->callback_cmd) {
		log_(t, "  Change success command:");
		log_(t, "    %s", cfg->callback_cmd);
	}

	if (cfg->laptop_lid_monitor == OFF) {
		log_(t, "  Laptop lid monitoring disabled");
	} else if (cfg->laptop_display_prefix) {
		log_(t, "  Laptop display prefix: %s", cfg->laptop_display_prefix);
	}
}

static void print_newline(const enum LogThreshold t, bool *print) {
	if (print && *print) {
		log_(t, NULL);
		*print = false;
	}
}

void print_cfg_commands(const enum LogThreshold t, const struct Cfg * const cfg) {
	if (!cfg)
		return;

	bool newline;

	if (cfg->align && cfg->arrange) {
		log_(t, NULL);
		log_(t, "way-displays -s ARRANGE_ALIGN %s %s", arrange_name(cfg->arrange), align_name(cfg->align));
	}

	if (sset_size(cfg->order_name_desc) > 0) {
		char *msg = NULL;

		for (const struct SSetIt *it = sset_it(cfg->order_name_desc); it; it = sset_it_next(it)) {
			msg = sprintf_append(msg, "'%s' ", it->val);
		}

		log_(t, NULL);
		log_(t, "way-displays -s ORDER %s", msg);

		free(msg);
	}

	if (cfg->scaling) {
		log_(t, NULL);
		log_(t, "way-displays -s SCALING %s", on_off_name(cfg->scaling));
	}

	if (cfg->auto_scale) {
		log_(t, NULL);
		log_(t, "way-displays -s AUTO_SCALE %s", on_off_name(cfg->auto_scale));
	}

	newline = true;
	for (const struct SMapIIt *it = smapi_it(cfg->scales); it; it = smapi_it_next(it)) {
		char *msg = sprintf_alloc("%.3f", (double)it->val/1000);
		print_newline(t, &newline);
		log_(t, "way-displays -s SCALE '%s' %s", it->key, msg);
		free(msg);
	}

	newline = true;

	for (const struct SMapIt *it = smap_it(cfg->user_modes); it; it = smap_it_next(it)) {
		struct UserMode *user_mode = (struct UserMode*)it->val;

		char *msg;
		if (user_mode->max) {
			msg = sprintf_alloc("MAX");
		} else if (user_mode->refresh_mhz != -1) {
			msg = sprintf_alloc("%d %d %s", user_mode->width, user_mode->height, mhz_to_hz_str(user_mode->refresh_mhz));
		} else {
			msg = sprintf_alloc("%d %d", user_mode->width, user_mode->height);
		}

		print_newline(t, &newline);
		log_(t, "way-displays -s MODE '%s' %s", it->key, msg);
		free(msg);
	}

	newline = true;
	for (const struct SMapIIt *it = smapi_it(cfg->transforms); it; it = smapi_it_next(it)) {
		print_newline(t, &newline);
		log_(t, "way-displays -s TRANSFORM '%s' %s", it->key, transform_name(it->val));
	}

	newline = true;
	for (const struct PSetIt *it = pset_it(cfg->disableds); it; it = pset_it_next(it)) {
		const struct Disabled* d = it->val;
		if (pset_size(d->conditions) == 0) {
			print_newline(t, &newline);
			log_(t, "way-displays -s DISABLED '%s'", d->name_desc);
		}
	}

	newline = true;
	for (const struct SSetIt *it = sset_it(cfg->adaptive_sync_off); it; it = sset_it_next(it)) {
		print_newline(t, &newline);
		log_(t, "way-displays -s VRR_OFF '%s'", it->val);
	}

	newline = true;
	if (cfg->callback_cmd) {
		log_(t, NULL);
		log_(t, "way-displays -s CALLBACK_CMD '%s'", cfg->callback_cmd);
	}
}

void print_head_current(const enum LogThreshold t, const struct Head * const head) {

	if (!head)
		return;

	if (head->current.enabled) {
		log_(t, "    scale:     %.3f (%.3f)", wl_fixed_to_double(head->current.scale), mode_scale(head->current.wlr_mode));

		const struct Output *output = imap_match_val(g_outputs, (fn_match_ptr)output_matches_name, head->name).val;
		if (output) {
			log_(t, "    size:      %dx%d", output->logical_width, output->logical_height);
			log_(t, "    position:  %d,%d", output->logical_x, output->logical_y);
		} else {
			log_(t, "    position:  %d,%d", head->current.x, head->current.y);
		}

		if (head->current.transform) {
			log_(t, "    transform: %s", transform_name(head->current.transform));
		}
	}

	print_wlr_mode(t, head->current.wlr_mode);
	log_(t, "    VRR:       %s", head->current.adaptive_sync == ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED ? "on" : "off");

	if (head->current.enabled) {
		if (head->overrided_enabled == OverrideTrue) {
			log_(t, "    (manually enabled)");
		}
	} else {
		if (head->overrided_enabled == OverrideFalse) {
			log_(t, "    (manually disabled)");
		} else {
			log_(t, "    (disabled)");
		}
	}

	if (lid_is_closed(head->name)) {
		log_(t, "    (lid closed)");
	}
}

void print_head_desired(const enum LogThreshold t, const struct Head * const head) {
	if (!head)
		return;

	if (head->desired.enabled) {
		if (head_current_mode_not_desired(head)) {
			// mode changes happen in their own operation
			if (!head->current.enabled || head->current.wlr_mode != head->desired.wlr_mode) {
				print_wlr_mode(t, head->desired.wlr_mode);
			}
		} else if (head_current_adaptive_sync_not_desired(head)) {
			// adaptive sync changes happen in their own operation
			log_(t, "    VRR:       %s", head->desired.adaptive_sync == ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED ? "on" : "off");
		} else {
			if (!head->current.enabled || head->current.scale != head->desired.scale) {
				log_(t, "    scale:     %.3f%s",
						wl_fixed_to_double(head->desired.scale),
						(!head->width_mm || !head->height_mm) ? " (default, size not specified)" : ""
					);
			}
			if (!head->current.enabled || head->current.x != head->desired.x || head->current.y != head->desired.y) {
				log_(t, "    position:  %d,%d",
						head->desired.x,
						head->desired.y
					);
			}
			if (!head->current.enabled || head->current.transform != head->desired.transform) {
				if (head->desired.transform) {
					log_(t, "    transform: %s", transform_name(head->desired.transform));
				} else {
					log_(t, "    transform: none");
				}
			}
		}
		if (!head->current.enabled) {
			if (head->overrided_enabled == OverrideTrue) {
				log_(t, "    (manually enabled)");
			} else {
				log_(t, "    (enabled)");
			}
		}
	} else {
		if (head->overrided_enabled == OverrideFalse) {
			log_(t, "    (manually disabled)");
		} else {
			log_(t, "    (disabled)");
		}
	}
}

void print_head(const enum LogThreshold t, const enum InfoEvent event, const struct Head * const head) {
	if (!head)
		return;

	const struct WlrMode *preferred_mode = head_preferred_wlr_mode(head);

	switch (event) {
		case ARRIVED:
		case NONE:
			log_(t, NULL);
			log_(t, "%s%s:", head->name ? head->name : "???", event == ARRIVED ? " Arrived" : "");
			log_(t, "  info:");
			if (head->name)
				log_(t, "    name:      '%s'", head->name);
			if (head->make)
				log_(t, "    make:      '%s'", head->make);
			if (head->model)
				log_(t, "    model:     '%s'", head->model);
			if (head->serial_number)
				log_(t, "    serial:    '%s'", head->serial_number);
			if (head->description)
				log_(t, "    desc:      '%s'", head->description);
			if (head->width_mm && head->height_mm) {
				log_(t, "    width:     %dmm", head->width_mm);
				log_(t, "    height:    %dmm", head->height_mm);
				if (preferred_mode) {
					log_(t, "    dpi:       %.2f @ %dx%d", mode_dpi(preferred_mode), preferred_mode->width, preferred_mode->height);
				}
			} else {
				log_(t, "    width:     (not specified)");
				log_(t, "    height:    (not specified)");
			}
			print_modes_res_refresh(t, head);
			print_modes_failed(t, head);
			log_(t, "  current:");
			print_head_current(t, head);
			break;
		case DEPARTED:
			log_(t, NULL);
			log_(t, "%s Departed:", head->name);
			if (head->name)
				log_(t, "    name:      '%s'", head->name);
			if (head->description)
				log_(t, "    desc:      '%s'", head->description);
			break;
		case DELTA:
			if (head_current_not_desired(head) || head_reapply_required(head)) {
				log_(t, NULL);
				log_(t, "%s Changing:", head->name);
				log_(t, "  from:");
				print_head_current(t, head);
				log_(t, "  to:");
				print_head_desired(t, head);
			}
			break;
	}
}

void print_heads(const enum LogThreshold t, const enum InfoEvent event, const struct SList * const heads) {
	for (const struct SList *i = heads; i; i = i->nex) {
		print_head(t, event, i->val);
	}
}

void print_list(const enum LogThreshold t, const struct SList * const heads) {
	if (!heads)
		return;

	size_t max_len_human = 0;
	for (const struct SList *i = heads; i; i = i->nex) {
		max_len_human = MAX(strlen(head_human(i->val)), max_len_human);
	}

	for (const struct SList *i = heads; i; i = i->nex) {
		struct Head *head = i->val;

		if (head->current.enabled && head->current.wlr_mode) {
			// full info
			log_(t, "%-*.*s %.3f %s %5d x%5d @%4d Hz",
					(int)max_len_human, (int)max_len_human, head_human(head),
					wl_fixed_to_double(head->current.scale),
					(head->current.adaptive_sync == ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED) ? "VRR" : "",
					head->current.wlr_mode->width,
					head->current.wlr_mode->height,
					mhz_to_hz_rounded(head->current.wlr_mode->refresh_mhz)
				);
		} else {
			// no mode is considered disabled
			log_(t, "%-*.*s disabled",
					(int)max_len_human, (int)max_len_human, head_human(head)
				);
		}
	}
}

void print_adaptive_sync_fail(const enum LogThreshold t, const struct Head * const head) {
	if (!head) {
		return;
	}

	log_(t, NULL);
	log_(t, "%s:", head_human(head));
	log_(t, "  Cannot enable VRR: this display or compositor may not support it.");
	log_(t, "  To speed things up you can disable VRR for this display by adding the following or similar to your cfg.yaml");
	log_(t, "  VRR_OFF:");
	log_(t, "    - '%s'", head->model ? head->model : "name_desc");
}

void print_mode_fail(const enum LogThreshold t, const struct Head * const head, const struct WlrMode * const wlr_mode) {
	log_(t, NULL);
	log_(t, "Changes failed");

	if (!head) {
		return;
	}

	log_(t, "  %s:", head_human(head));
	print_wlr_mode(t, wlr_mode);
}

char *delta_human(const struct SList * const heads) {
	if (!heads) {
		return NULL;
	}

	char *delta = NULL;

	for (const struct SList *i = heads; i; i = i->nex) {
		const struct Head * head = i->val;

		// disable in own operation
		if (head->current.enabled && !head->desired.enabled) {
			delta = sprintf_append(delta, "%s\n  disabled\n", head_human(head));
			continue;
		}

		// enable in own operation
		if (!head->current.enabled && head->desired.enabled) {
			delta = sprintf_append(delta, "%s\n  enabled\n", head_human(head));
			continue;
		}

		if (head_current_not_desired(head)) {
			delta = sprintf_append(delta, "%s\n", head_human(head));

			if (head->current.scale != head->desired.scale) {
				delta = sprintf_append(delta, "  scale:     %.3f -> %.3f\n",
						wl_fixed_to_double(head->current.scale),
						wl_fixed_to_double(head->desired.scale)
						);
			}

			if (head->current.transform != head->desired.transform) {
				delta = sprintf_append(delta, "  transform: %s -> %s\n",
						head->current.transform ? transform_name(head->current.transform) : "none",
						head->desired.transform ? transform_name(head->desired.transform) : "none"
						);
			}

			if (head->current.x != head->desired.x || head->current.y != head->desired.y) {
				delta = sprintf_append(delta, "  position:  %d,%d -> %d,%d\n",
						head->current.x, head->current.y,
						head->desired.x, head->desired.y
						);
			}
		}
	}

	// strip trailing newline
	if (delta) {
		size_t len = strlen(delta);
		if (len > 0) {
			delta[len - 1] = '\0';
		}
	}

	return delta;
}

char *delta_human_mode(const struct Head * const head) {
	if (!head) {
		return NULL;
	}

	char *delta = NULL;

	delta = sprintf_append(delta, "%s\n  ",
			head_human(head)
			);

	if (head->current.wlr_mode) {
		delta = sprintf_append(delta, "%dx%d@%dHz -> ",
				head->current.wlr_mode->width,
				head->current.wlr_mode->height,
				mhz_to_hz_rounded(head->current.wlr_mode->refresh_mhz)
				);
	} else {
		delta = sprintf_append(delta, "(no mode) -> ");
	}

	if (head->desired.wlr_mode) {
		delta = sprintf_append(delta, "%dx%d@%dHz",
				head->desired.wlr_mode->width,
				head->desired.wlr_mode->height,
				mhz_to_hz_rounded(head->desired.wlr_mode->refresh_mhz)
				);
	} else {
		delta = sprintf_append(delta, "(no mode)");
	}

	return delta;
}


char *delta_human_adaptive_sync(const struct Head * const head) {
	if (!head) {
		return NULL;
	}

	return sprintf_append(NULL, "%s\n  VRR %s",
			head_human(head),
			head->desired.adaptive_sync == ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED ? "on" : "off"
			);
}

char *delta_human_reapply(const struct Head * const head) {
	if (!head)
		return NULL;

	return sprintf_alloc("%s\n  disabled\n  modes reset",
			head_human(head)
			);
}

void call_back(const enum LogThreshold t, const char * const msg1, const char * const msg2) {
	if (!g_cfg->callback_cmd || t < log_get_threshold()) {
		return;
	}

	log_info(NULL);
	log_info("Executing CALLBACK_CMD:");
	log_info("  %s", g_cfg->callback_cmd);

	// decorate human message and optional log
	char *buf = (char*)calloc(CALLBACK_MSG_LEN, sizeof(char));
	snprintf(buf, CALLBACK_MSG_LEN, "%s%s", msg1 ? msg1 : "", msg2 ? msg2 : "");

	// pack environment variables
	const struct SMapS *env = smaps_init();

	smaps_put_if_absent(env, "CALLBACK_MSG", buf);
	smaps_put_if_absent(env, "CALLBACK_LEVEL", log_threshold_name(t));

	char *env_str = smaps_str(env);
	log_debug("%s", env_str);
	free(env_str);

	// execute callback
	spawn_sh_cmd(g_cfg->callback_cmd, env);

	smaps_free(env);
	free(buf);
}

void call_back_mode_fail(const enum LogThreshold t, const struct Head * const head, const struct WlrMode * const wlr_mode) {
	if (!head) {
		return;
	}

	char *mode_str = info_wlr_mode_string(wlr_mode);

	char *human = sprintf_alloc(
			"%s\n"
			"  Unable to set mode %s, retrying",
			head_human(head),
			mode_str);

	call_back(t, human, NULL);

	free(mode_str);
	free(human);
}

void call_back_adaptive_sync_fail(const enum LogThreshold t, const struct Head * const head) {
	if (!g_cfg->callback_cmd || !head) {
		return;
	}

	// custom human message
	char *human = sprintf_alloc(
			"%s\n"
			"  Cannot enable VRR.\n"
			"  You can disable VRR for this display in cfg.yaml\n"
			"VRR_OFF:\n"
			"  - '%s'",
			head_human(head),
			head->model ? head->model : "name_desc");

	call_back(t, human, NULL);

	free(human);
}

