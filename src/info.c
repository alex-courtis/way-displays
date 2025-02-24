#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <wayland-util.h>

#include "info.h"

#include "cfg.h"
#include "convert.h"
#include "displ.h"
#include "global.h"
#include "head.h"
#include "lid.h"
#include "log.h"
#include "mode.h"
#include "output.h"
#include "process.h"
#include "slist.h"
#include "stable.h"
#include "wlr-output-management-unstable-v1.h"

void info_user_mode_string(const struct UserMode * const user_mode, char * const buf, const size_t nbuf) {
	if (!user_mode) {
		*buf = '\0';
		return;
	}

	if (user_mode->max) {
		snprintf(buf, nbuf, "MAX");
	} else if (user_mode->refresh_mhz != -1) {
		snprintf(buf, nbuf, "%dx%d@%sHz",
				user_mode->width,
				user_mode->height,
				mhz_to_hz_str(user_mode->refresh_mhz)
				);
	} else {
		snprintf(buf, nbuf, "%dx%d",
				user_mode->width,
				user_mode->height
				);
	}
}

void info_mode_string(const struct Mode * const mode, char *buf, const size_t nbuf) {
	if (!mode) {
		*buf = '\0';
		return;
	}

	snprintf(buf, nbuf, "%dx%d@%dHz (%d,%03dmHz)%s",
			mode->width,
			mode->height,
			mhz_to_hz_rounded(mode->refresh_mhz),
			mode->refresh_mhz / 1000,
			mode->refresh_mhz % 1000,
			mode->preferred ? " (preferred)" : ""
			);
}

void print_user_mode(const enum LogThreshold t, const struct UserMode * const user_mode, const bool del) {
	if (!user_mode)
		return;

	static char buf[512];

	if (del) {
		log_(t, "    %s", user_mode->name_desc);
	} else {
		info_user_mode_string(user_mode, buf, sizeof(buf));
		log_(t, "    %s: %s", user_mode->name_desc, buf);
	}
}

void print_mode(const enum LogThreshold t, const struct Mode * const mode) {
	static char buf[2048];

	if (mode) {
		info_mode_string(mode, buf, sizeof(buf));
		log_(t, "    mode:      %s", buf);
	} else {
		log_(t, "    (no mode)");
	}
}

void print_modes_failed(const enum LogThreshold t, const struct Head * const head) {
	if (!head)
		return;

	if (head->modes_failed) {
		log_(t, "  failed:");
		for (struct SList *i = head->modes_failed; i; i = i->nex) {
			print_mode(t, i->val);
		}
	}
}

void print_modes_res_refresh(const enum LogThreshold t, const struct Head * const head) {
	if (!head)
		return;

	static char buf[2048];
	char *bp;

	struct SList *mrrs = modes_res_refresh(head->modes);
	struct Mode *preferred_mode = head_preferred_mode(head);

	struct ModesResRefresh *mrr = NULL;
	struct Mode *mode = NULL;

	for (struct SList *i = mrrs; i; i = i->nex) {
		mrr = i->val;

		bp = buf;
		bp += snprintf(bp, sizeof(buf) - (bp - buf), "    mode:     %5d x%5d @%4d Hz ", mrr->width, mrr->height, mhz_to_hz_rounded(mrr->refresh_mhz));

		for (struct SList *j = mrr->modes; j; j = j->nex) {
			mode = j->val;
			bp += snprintf(bp, sizeof(buf) - (bp - buf), "%4d,%03d mHz", mode->refresh_mhz / 1000, mode->refresh_mhz % 1000);
			if (mode == preferred_mode) {
				bp += snprintf(bp, sizeof(buf) - (bp - buf), " (preferred)");
			}
		}
		log_(t,"%s", buf);
	}

	slist_free_vals(&mrrs, mode_res_refresh_free);
}

void print_cfg(const enum LogThreshold t, const struct Cfg * const cfg, const bool del) {
	if (!cfg)
		return;

	struct SList *i;

	if (cfg->arrange && cfg->align) {
		log_(t, "  Arrange in a %s aligned at the %s", arrange_name(cfg->arrange), align_name(cfg->align));
	} else if (cfg->arrange) {
		log_(t, "  Arrange in a %s", arrange_name(cfg->arrange));
	} else if (cfg->align) {
		log_(t, "  Align at the %s", align_name(cfg->align));
	}

	if (cfg->order_name_desc) {
		log_(t, "  Order:");
		for (i = cfg->order_name_desc; i; i = i->nex) {
			log_(t, "    %s", (char*)i->val);
		}
	}

	if (cfg->scaling) {
		log_(t, "  Scaling: %s", on_off_name(cfg->scaling));
	}

	if (cfg->auto_scale) {
		if (cfg->auto_scale_max > 0) {
			log_(t, "  Auto scale: %s (min: %0.3f, max: %0.3f)",
					on_off_name(cfg->auto_scale), cfg->auto_scale_min, cfg->auto_scale_max);
		} else {
			log_(t, "  Auto scale: %s (min: %0.3f)",
					on_off_name(cfg->auto_scale), cfg->auto_scale_min);
		}
	}

	if (cfg->user_scales) {
		log_(t, "  Scale:");
		struct UserScale *user_scale;
		for (i = cfg->user_scales; i; i = i->nex) {
			user_scale = (struct UserScale*)i->val;
			if (del) {
				log_(t, "    %s", user_scale->name_desc);
			} else {
				log_(t, "    %s: %.3f", user_scale->name_desc, user_scale->scale);
			}
		}
	}

	if (cfg->user_modes) {
		log_(t, "  Mode:");
		struct UserMode *user_mode;
		for (i = cfg->user_modes; i; i = i->nex) {
			user_mode = (struct UserMode*)i->val;
			print_user_mode(t, user_mode, del);
		}
	}

	if (cfg->user_transforms) {
		log_(t, "  Transform:");
		struct UserTransform *user_transform;
		for (i = cfg->user_transforms; i; i = i->nex) {
			user_transform = (struct UserTransform*)i->val;
			if (del) {
				log_(t, "    %s", user_transform->name_desc);
			} else {
				log_(t, "    %s: %s", user_transform->name_desc, transform_name(user_transform->transform));
			}
		}
	}

	if (cfg->max_preferred_refresh_name_desc) {
		log_(t, "  Max preferred refresh:");
		for (i = cfg->max_preferred_refresh_name_desc; i; i = i->nex) {
			log_(t, "    %s", (char*)i->val);
		}
	}

	if (cfg->disabled_name_desc) {
		log_(t, "  Disabled:");
		for (i = cfg->disabled_name_desc; i; i = i->nex) {
			log_(t, "    %s", (char*)i->val);
		}
	}

	if (cfg->change_success_cmd) {
		log_(t, "  Change success command:");
		log_(t, "    %s", cfg->change_success_cmd);
	}

	if (cfg->laptop_display_prefix) {
		log_(t, "  Laptop display prefix: %s", cfg->laptop_display_prefix);
	}
}

void print_newline(const enum LogThreshold t, bool *print) {
	if (print && *print) {
		log_(t, "");
		*print = false;
	}
}

void print_cfg_commands(const enum LogThreshold t, const struct Cfg * const cfg) {
	if (!cfg)
		return;

	static char buf[2048];
	char *bp;

	struct SList *i = NULL;
	bool newline;

	if (cfg->align && cfg->arrange) {
		log_(t, "\nway-displays -s ARRANGE_ALIGN %s %s", arrange_name(cfg->arrange), align_name(cfg->align));
	}

	if (cfg->order_name_desc) {
		bp = buf;
		*bp = '\0';

		for (i = cfg->order_name_desc; i; i = i->nex) {
			bp += snprintf(bp, sizeof(buf) - (bp - buf), "'%s' ", (char*)i->val);
		}

		log_(t, "\nway-displays -s ORDER %s", buf);
	}

	if (cfg->scaling) {
		log_(t, "\nway-displays -s SCALING %s", on_off_name(cfg->scaling));
	}

	if (cfg->auto_scale) {
		log_(t, "\nway-displays -s AUTO_SCALE %s", on_off_name(cfg->auto_scale));
	}

	newline = true;
	for (i = cfg->user_scales; i; i = i->nex) {
		struct UserScale *user_scale = (struct UserScale*)i->val;
		snprintf(buf, sizeof(buf), "%.3f", user_scale->scale);
		print_newline(t, &newline);
		log_(t, "way-displays -s SCALE '%s' %s", user_scale->name_desc, buf);
	}

	newline = true;
	for (i = cfg->user_modes; i; i = i->nex) {
		struct UserMode *user_mode = (struct UserMode*)i->val;

		if (user_mode->max) {
			snprintf(buf, sizeof(buf), "MAX");
		} else if (user_mode->refresh_mhz != -1) {
			snprintf(buf, sizeof(buf), "%d %d %s", user_mode->width, user_mode->height, mhz_to_hz_str(user_mode->refresh_mhz));
		} else {
			snprintf(buf, sizeof(buf), "%d %d", user_mode->width, user_mode->height);
		}

		print_newline(t, &newline);
		log_(t, "way-displays -s MODE '%s' %s", user_mode->name_desc, buf);
	}

	newline = true;
	for (i = cfg->user_transforms; i; i = i->nex) {
		struct UserTransform *user_transform = (struct UserTransform*)i->val;

		print_newline(t, &newline);
		log_(t, "way-displays -s TRANSFORM '%s' %s", user_transform->name_desc, transform_name(user_transform->transform));
	}

	newline = true;
	for (i = cfg->disabled_name_desc; i; i = i->nex) {
		print_newline(t, &newline);
		log_(t, "way-displays -s DISABLED '%s'", (char*)i->val);
	}

	newline = true;
	for (i = cfg->adaptive_sync_off_name_desc; i; i = i->nex) {
		print_newline(t, &newline);
		log_(t, "way-displays -s VRR_OFF '%s'", (char*)i->val);
	}

	newline = true;
	if (cfg->change_success_cmd) {
		print_newline(t, &newline);
		log_(t, "way-displays -s CALLBACK_CMD '%s'", cfg->change_success_cmd);
	}
}

void print_head_current(const enum LogThreshold t, const struct Head * const head) {
	static const struct Output *output = NULL;

	if (!head)
		return;

	if (head->current.enabled) {
		log_(t, "    scale:     %.3f (%.3f)", wl_fixed_to_double(head->current.scale), mode_scale(head->current.mode));

		if ((output = output_for_name(head->name))) {
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

	if (!head->current.enabled) {
		log_(t, "    (disabled)");
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
			log_(t, "    (enabled)");
		}
	} else {
		log_(t, "    (disabled)");
	}
}

void print_head(const enum LogThreshold t, const enum InfoEvent event, const struct Head * const head) {
	if (!head)
		return;

	struct Mode *preferred_mode = head_preferred_mode(head);

	switch (event) {
		case ARRIVED:
		case NONE:
			log_(t, "\n%s%s:", head->name ? head->name : "???", event == ARRIVED ? " Arrived" : "");
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
			log_(t, "\n%s Departed:", head->name);
			if (head->name)
				log_(t, "    name:      '%s'", head->name);
			if (head->description)
				log_(t, "    desc:      '%s'", head->description);
			break;
		case DELTA:
			if (head_current_not_desired(head)) {
				log_(t, "\n%s Changing:", head->name);
				log_(t, "  from:");
				print_head_current(t, head);
				log_(t, "  to:");
				print_head_desired(t, head);
			}
			break;
		default:
			break;
	}
}

void print_heads(const enum LogThreshold t, const enum InfoEvent event, const struct SList * const heads) {
	for (const struct SList *i = heads; i; i = i->nex) {
		print_head(t, event, i->val);
	}
}

void print_adaptive_sync_fail(const enum LogThreshold t, const struct Head * const head) {
	if (!head) {
		return;
	}

	log_(t, "\n%s:", head_human(head));
	log_(t, "  Cannot enable VRR: this display or compositor may not support it.");
	log_(t, "  To speed things up you can disable VRR for this display by adding the following or similar to your cfg.yaml");
	log_(t, "  VRR_OFF:");
	log_(t, "    - '%s'", head->model ? head->model : "name_desc");
}

void print_mode_fail(const enum LogThreshold t, const struct Head * head, const struct Mode * const mode) {
	log_(t, "\nChanges failed");

	if (!head) {
		return;
	}

	log_(t, "  %s:", head_human(head));
	print_mode(t, mode);
}

char *delta_human(const enum DisplState state, const struct SList * const heads) {
	if (!heads) {
		return NULL;
	}

	char *buf = (char*)calloc(CALLBACK_MSG_LEN, sizeof(char));
	char *bufp = buf;

	for (const struct SList *i = heads; i; i = i->nex) {
		const struct Head * head = i->val;

		// disable in own operation
		if (head->current.enabled && !head->desired.enabled) {
			bufp += snprintf(bufp, CALLBACK_MSG_LEN - (bufp - buf), "%s\n  disabled\n", head_human(head));
			continue;
		}

		// enable in own operation
		if (!head->current.enabled && head->desired.enabled) {
			bufp += snprintf(bufp, CALLBACK_MSG_LEN - (bufp - buf), "%s\n  enabled\n", head_human(head));
			continue;
		}

		if (head_current_not_desired(head)) {
			bufp += snprintf(bufp, CALLBACK_MSG_LEN - (bufp - buf), "%s\n", head_human(head));

			if (head->current.scale != head->desired.scale) {
				bufp += snprintf(bufp, CALLBACK_MSG_LEN - (bufp - buf), "  scale:     %.3f -> %.3f\n",
						wl_fixed_to_double(head->current.scale),
						wl_fixed_to_double(head->desired.scale)
						);
			}

			if (head->current.transform != head->desired.transform) {
				bufp += snprintf(bufp, CALLBACK_MSG_LEN - (bufp - buf), "  transform: %s -> %s\n",
						head->current.transform ? transform_name(head->current.transform) : "none",
						head->desired.transform ? transform_name(head->desired.transform) : "none"
						);
			}

			if (head->current.x != head->desired.x || head->current.y != head->desired.y) {
				bufp += snprintf(bufp, CALLBACK_MSG_LEN - (bufp - buf), "  position:  %d,%d -> %d,%d\n",
						head->current.x, head->current.y,
						head->desired.x, head->desired.y
						);
			}
		}
	}

	// strip trailing newline
	if (bufp > buf) {
		*(bufp - 1) = '\0';
	}

	return buf;
}

char *delta_human_mode(const enum DisplState state, const struct Head * const head) {
	if (!head) {
		return NULL;
	}

	char *buf = (char*)calloc(CALLBACK_MSG_LEN, sizeof(char));
	char *bufp = buf;

	bufp += snprintf(bufp, CALLBACK_MSG_LEN - (bufp - buf), "%s\n  ",
			head_human(head)
			);

	if (head->current.mode) {
		bufp += snprintf(bufp, CALLBACK_MSG_LEN - (bufp - buf), "%dx%d@%dHz -> ",
				head->current.mode->width,
				head->current.mode->height,
				mhz_to_hz_rounded(head->current.mode->refresh_mhz)
				);
	} else {
		bufp += snprintf(bufp, CALLBACK_MSG_LEN - (bufp - buf), "(no mode) -> ");
	}

	if (head->desired.mode) {
		bufp += snprintf(bufp, CALLBACK_MSG_LEN - (bufp - buf), "%dx%d@%dHz",
				head->desired.mode->width,
				head->desired.mode->height,
				mhz_to_hz_rounded(head->desired.mode->refresh_mhz)
				);
	} else {
		bufp += snprintf(bufp, CALLBACK_MSG_LEN - (bufp - buf), "(no mode)");
	}

	return buf;
}


char *delta_human_adaptive_sync(const enum DisplState state, const struct Head * const head) {
	if (!head) {
		return NULL;
	}

	char *buf = (char*)calloc(CALLBACK_MSG_LEN, sizeof(char));
	char *bufp = buf;

	bufp += snprintf(bufp, CALLBACK_MSG_LEN - (bufp - buf), "%s\n  VRR %s",
			head_human(head),
			head->desired.adaptive_sync == ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED ? "on" : "off"
			);

	return buf;
}

void call_back(const enum LogThreshold t, const char * const msg1, const char * const msg2) {
	if (!cfg->change_success_cmd) {
		return;
	}

	log_info("\nExecuting CALLBACK_CMD:");
	log_info("  %s", cfg->change_success_cmd);

	// decorate human message and optional log
	char *buf = (char*)calloc(CALLBACK_MSG_LEN, sizeof(char));
	snprintf(buf, CALLBACK_MSG_LEN, "%s%s", msg1 ? msg1 : "", msg2 ? msg2 : "");

	// pack environment variables
	const struct STable *env = stable_init(1, 1, false);
	stable_put(env, "CALLBACK_MSG", buf);
	stable_put(env, "CALLBACK_STATUS", log_threshold_name(t));

	// execute callback
	spawn_sh_cmd(cfg->change_success_cmd, env);

	stable_free(env);
	free(buf);
}

void call_back_mode_fail(const enum LogThreshold t, const struct Head * const head, const struct Mode * mode) {
	if (!head) {
		return;
	}

	// custom human message
	static char buf[2048];
	info_mode_string(mode, buf, sizeof(buf));

	char *human = (char*)calloc(CALLBACK_MSG_LEN, sizeof(char));

	snprintf(human, CALLBACK_MSG_LEN,
			"%s\n"
			"  Unable to set mode %s, retrying",
			head_human(head),
			buf);

	call_back(t, human, NULL);

	free(human);
}

void call_back_adaptive_sync_fail(const enum LogThreshold t, const struct Head * const head) {
	if (!cfg->change_success_cmd || !head) {
		return;
	}

	// custom human message
	char *human = (char*)calloc(CALLBACK_MSG_LEN, sizeof(char));

	snprintf(human, CALLBACK_MSG_LEN,
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

