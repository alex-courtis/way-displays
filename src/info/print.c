#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <wayland-util.h>

#include "info/print.h"

#include "cfg.h"
#include "cfg/disabled.h"
#include "convert.h"
#include "displ.h"
#include "fn.h"
#include "head.h"
#include "ipmap.h"
#include "lid.h"
#include "log.h"
#include "mode.h"
#include "output.h"
#include "pset.h"
#include "pslist.h"
#include "spmap.h"
#include "simap.h"
#include "sset.h"
#include "str.h"
#include "wlr-output-management-unstable-v1.h"

static void print_mode_cfg(const enum LogThreshold t, const char * name_desc, const struct Mode * const mode, const bool del) {
	if (!mode)
		return;

	if (del) {
		log_(t, "    %s", name_desc);
	} else {
		char *um_str = mode_str_brief(mode);
		log_(t, "    %s: %s", name_desc, um_str);
		free(um_str);
	}
}

static void print_mode(const enum LogThreshold t, const struct Mode * const mode) {

	if (mode) {
		char *str = mode_str(mode);
		log_(t, "    mode:      %s", str);
		free(str);
	} else {
		log_(t, "    (no mode)");
	}
}

static void print_modes_failed(const enum LogThreshold t, const struct Head * const head) {
	if (!head)
		return;

	if (pset_size(head->modes_failed) > 0) {
		log_(t, "  failed:");
		for (const struct PsetIt *it = pset_it(head->modes_failed); it; it = pset_it_next(it)) {
			print_mode(t, it->val);
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

	// show from the top down
	const struct Pset *modes_sorted = pset_clone(head->modes);
	pset_sort(modes_sorted, (fn_less_than)mode_greater_than_res_refresh);
	const struct PsetIt *it = pset_it(modes_sorted);
	while (it) {

		// res/refresh/hz line
		const struct Mode *mode_major = it->val;
		char *msg = sprintf_alloc("    mode:     %5d x%5d @%4d Hz ", mode_major->width, mode_major->height, mode_hz_rounded(mode_major));

		// append all modes matching the line
		const struct Mode *mode_minor = mode_major;
		while (mode_minor && mode_equal_res_hz(mode_major, mode_minor)) {

			// append mHz
			msg = sprintf_append(msg, "%4d,%03d mHz", mode_minor->refresh_mhz / 1000, mode_minor->refresh_mhz % 1000);

			// append preferred
			if (mode_minor == head->mode_preferred) {
				msg = sprintf_append(msg, " (preferred)");
			}

			it = pset_it_next(it);
			mode_minor = it ? it->val : NULL;
		}

		log_(t,"%s", msg);
		free(msg);
	}

	pset_free(modes_sorted);
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
		for (const struct SsetIt *it = sset_it(cfg->order_name_desc); it; it = sset_it_next(it)) {
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

	if (simap_size(cfg->scales) > 0) {
		log_(t, "  Scale:");
		for (const struct SImapIt *it = simap_it(cfg->scales); it; it = simap_it_next(it)) {
			if (del) {
				log_(t, "    %s", it->key);
			} else {
				log_(t, "    %s: %.3f", it->key, (double)it->val/1000);
			}
		}
	}

	if (spmap_size(cfg->modes) > 0) {
		log_(t, "  Mode:");
		for (const struct SPmapIt *it = spmap_it(cfg->modes); it; it = spmap_it_next(it)) {
			print_mode_cfg(t, it->key, it->val, del);
		}
	}

	if (simap_size(cfg->transforms) > 0) {
		log_(t, "  Transform:");
		for (const struct SImapIt *it = simap_it(cfg->transforms); it; it = simap_it_next(it)) {
			if (del) {
				log_(t, "    %s", it->key);
			} else {
				log_(t, "    %s: %s", it->key, transform_name(it->val));
			}
		}
	}

	if (sset_size(cfg->max_preferred_refresh) > 0) {
		log_(t, "  Max preferred refresh:");
		for (const struct SsetIt *it = sset_it(cfg->max_preferred_refresh); it; it = sset_it_next(it)) {
			log_(t, "    %s", it->val);
		}
	}

	if (pset_size(cfg->disableds) > 0) {
		log_(t, "  Disabled:");
		for (const struct PsetIt *it = pset_it(cfg->disableds); it; it = pset_it_next(it)) {
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

		for (const struct SsetIt *it = sset_it(cfg->order_name_desc); it; it = sset_it_next(it)) {
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
	for (const struct SImapIt *it = simap_it(cfg->scales); it; it = simap_it_next(it)) {
		char *msg = sprintf_alloc("%.3f", (double)it->val/1000);
		print_newline(t, &newline);
		log_(t, "way-displays -s SCALE '%s' %s", it->key, msg);
		free(msg);
	}

	newline = true;

	for (const struct SPmapIt *it = spmap_it(cfg->modes); it; it = spmap_it_next(it)) {
		struct Mode *mode = (struct Mode*)it->val;

		char *msg;
		if (mode->max) {
			msg = sprintf_alloc("MAX");
		} else if (mode->refresh_mhz != -1) {
			msg = sprintf_alloc("%d %d %g", mode->width, mode->height, ((float)mode->refresh_mhz) / 1000);
		} else {
			msg = sprintf_alloc("%d %d", mode->width, mode->height);
		}

		print_newline(t, &newline);
		log_(t, "way-displays -s MODE '%s' %s", it->key, msg);
		free(msg);
	}

	newline = true;
	for (const struct SImapIt *it = simap_it(cfg->transforms); it; it = simap_it_next(it)) {
		print_newline(t, &newline);
		log_(t, "way-displays -s TRANSFORM '%s' %s", it->key, transform_name(it->val));
	}

	newline = true;
	for (const struct PsetIt *it = pset_it(cfg->disableds); it; it = pset_it_next(it)) {
		const struct Disabled* d = it->val;
		if (pset_size(d->conditions) == 0) {
			print_newline(t, &newline);
			log_(t, "way-displays -s DISABLED '%s'", d->name_desc);
		}
	}

	newline = true;
	for (const struct SsetIt *it = sset_it(cfg->adaptive_sync_off); it; it = sset_it_next(it)) {
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
		log_(t, "    scale:     %.3f (%.3f)", wl_fixed_to_double(head->current.scale), mode_scale(head->current.mode));

		const struct Output *output = ipmap_find_val(g_outputs, (fn_2pred)output_matches_name, head->name).val;
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

	print_mode(t, head->current.mode);
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

	if (g_lid_is_closed(head->name)) {
		log_(t, "    (lid closed)");
	}
}

void print_head_desired(const enum LogThreshold t, const struct Head * const head) {
	if (!head)
		return;

	if (head->desired.enabled) {
		if (head_current_mode_not_desired(head)) {
			// mode changes happen in their own operation
			if (!head->current.enabled || head->current.mode != head->desired.mode) {
				print_mode(t, head->desired.mode);
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
				if (head->mode_preferred) {
					log_(t, "    dpi:       %.2f @ %dx%d", mode_dpi(head->mode_preferred), head->mode_preferred->width, head->mode_preferred->height);
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

void print_heads(const enum LogThreshold t, const enum InfoEvent event, const struct Pslist * const heads) {
	for (const struct Pslist *i = heads; i; i = i->nex) {
		print_head(t, event, i->val);
	}
}

void print_list(const enum LogThreshold t, const struct Pslist * const heads) {
	if (!heads)
		return;

	size_t max_len_human = 0;
	for (const struct Pslist *i = heads; i; i = i->nex) {
		max_len_human = MAX(strlen(head_human(i->val)), max_len_human);
	}

	for (const struct Pslist *i = heads; i; i = i->nex) {
		struct Head *head = i->val;

		if (head->current.enabled && head->current.mode) {
			// full info
			log_(t, "%-*.*s %.3f %s %5d x%5d @%4d Hz",
					(int)max_len_human, (int)max_len_human, head_human(head),
					wl_fixed_to_double(head->current.scale),
					(head->current.adaptive_sync == ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED) ? "VRR" : "",
					head->current.mode->width,
					head->current.mode->height,
					mode_hz_rounded(head->current.mode)
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

void print_mode_fail(const enum LogThreshold t, const struct Head * const head, const struct Mode * const mode) {
	log_(t, NULL);
	log_(t, "Changes failed");

	if (!head) {
		return;
	}

	log_(t, "  %s:", head_human(head));
	print_mode(t, mode);
}

void print_head_queue(const enum LogThreshold t, const char *msg, enum DisplState displ_state, struct Pslist * const heads) {
	if (log_get_threshold() > DEBUG)
		return;

	char *reapply = strdup("");
	char *mode = strdup("");
	char *vrr = strdup("");
	char *remainder = strdup("");

	for (struct Pslist *i = heads; i; i = i->nex) {
		struct Head *head = i->val;

		// granular reapplies first
		if (head_reapply_required(head))
			reapply = sprintf_append(reapply, " %s:reapply ;", head->name);

		// granular mode
		if (head_current_mode_not_desired(head))
			mode = sprintf_append(mode, " %s:mode ;", head->name);

		// granular vrr
		if (head_current_adaptive_sync_not_desired(head))
			vrr = sprintf_append(vrr, " %s:vrr ;", head->name);

		// mass disable
		if (head->current.enabled && !head->desired.enabled)
			remainder = sprintf_append(remainder, " %s:disable", head->name);

		// mass enable
		if (!head->current.enabled && head->desired.enabled)
			remainder = sprintf_append(remainder, " %s:enable", head->name);

		// mass remainder
		if (head->desired.scale != head->current.scale ||
				head->desired.x != head->current.x ||
				head->desired.y != head->current.y ||
				head->desired.transform != head->current.transform )
			remainder = sprintf_append(remainder, " %s:geometry", head->name);
	}

	log_(t, "%s %s queue%s%s%s%s", msg, displ_state_name(displ_state), reapply, mode, vrr, remainder);

	free(reapply);
	free(mode);
	free(vrr);
	free(remainder);
}
