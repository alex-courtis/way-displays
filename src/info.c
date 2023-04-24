#include <stdbool.h>
#include <stdio.h>
#include <wayland-util.h>

#include "info.h"

#include "cfg.h"
#include "convert.h"
#include "head.h"
#include "lid.h"
#include "list.h"
#include "log.h"
#include "mode.h"
#include "wlr-output-management-unstable-v1.h"

void info_user_mode_string(struct UserMode *user_mode, char *buf, size_t nbuf) {
	if (!user_mode) {
		*buf = '\0';
		return;
	}

	if (user_mode->max) {
		snprintf(buf, nbuf, "MAX");
	} else if (user_mode->refresh_hz != -1) {
		snprintf(buf, nbuf, "%dx%d@%dHz",
				user_mode->width,
				user_mode->height,
				user_mode->refresh_hz
				);
	} else {
		snprintf(buf, nbuf, "%dx%d",
				user_mode->width,
				user_mode->height
				);
	}
}

void mode_string(struct Mode *mode, char *buf, size_t nbuf) {
	if (!mode) {
		*buf = '\0';
		return;
	}

	snprintf(buf, nbuf, "%dx%d@%dHz (%d,%03dmHz) %s",
			mode->width,
			mode->height,
			mhz_to_hz(mode->refresh_mhz),
			mode->refresh_mhz / 1000,
			mode->refresh_mhz % 1000,
			mode->preferred ? "(preferred)" : ""
			);
}

void print_user_mode(enum LogThreshold t, struct UserMode *user_mode, bool del) {
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

void print_mode(enum LogThreshold t, struct Mode *mode) {
	static char buf[2048];

	if (mode) {
		mode_string(mode, buf, sizeof(buf));
		log_(t, "    mode:     %s", buf);
	} else {
		log_(t, "    (no mode)");
	}
}

void print_modes_failed(enum LogThreshold t, struct Head *head) {
	if (!head)
		return;

	if (head->modes_failed) {
		log_(t, "  failed:");
		for (struct SList *i = head->modes_failed; i; i = i->nex) {
			print_mode(t, i->val);
		}
	}
}

void print_modes_res_refresh(enum LogThreshold t, struct Head *head) {
	if (!head)
		return;

	static char buf[2048];
	char *bp;

	struct SList *mrrs = modes_res_refresh(head->modes);

	struct ModesResRefresh *mrr = NULL;
	struct Mode *mode = NULL;

	for (struct SList *i = mrrs; i; i = i->nex) {
		mrr = i->val;

		bp = buf;
		bp += snprintf(bp, sizeof(buf) - (bp - buf), "    mode:    %5d x%5d @%4d Hz ", mrr->width, mrr->height, mrr->refresh_hz);

		for (struct SList *j = mrr->modes; j; j = j->nex) {
			mode = j->val;
			bp += snprintf(bp, sizeof(buf) - (bp - buf), "%4d,%03d mHz", mode->refresh_mhz / 1000, mode->refresh_mhz % 1000);
			if (mode == head->preferred_mode) {
				bp += snprintf(bp, sizeof(buf) - (bp - buf), " (preferred)");
			}
		}
		log_(t,"%s", buf);
	}

	slist_free_vals(&mrrs, mode_res_refresh_free);
}

void print_cfg(enum LogThreshold t, struct Cfg *cfg, bool del) {
	if (!cfg)
		return;

	struct UserScale *user_scale;
	struct UserMode *user_mode;
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

	if (cfg->auto_scale) {
		log_(t, "  Auto scale: %s", auto_scale_name(cfg->auto_scale));
	}

	if (cfg->user_scales) {
		log_(t, "  Scale:");
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
		for (i = cfg->user_modes; i; i = i->nex) {
			user_mode = (struct UserMode*)i->val;
			print_user_mode(t, user_mode, del);
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

	if (cfg->laptop_display_prefix) {
		log_(t, "  Laptop display prefix: %s", cfg->laptop_display_prefix);
	}
}

void print_head_current(enum LogThreshold t, struct Head *head) {
	if (!head)
		return;

	if (head->current.enabled) {
		log_(t, "    scale:    %.3f (%.3f)", wl_fixed_to_double(head->current.scale), mode_scale(head->current.mode));
		log_(t, "    position: %d,%d", head->current.x, head->current.y);
	}

	print_mode(t, head->current.mode);
	log_(t, "    VRR:      %s", head->current.adaptive_sync == ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED ? "on" : "off");

	if (!head->current.enabled) {
		log_(t, "    (disabled)");
	}

	if (lid_is_closed(head->name)) {
		log_(t, "    (lid closed)");
	}
}

void print_head_desired(enum LogThreshold t, struct Head *head) {
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
			log_(t, "    VRR:      %s", head->desired.adaptive_sync == ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED ? "on" : "off");
		} else {
			if (!head->current.enabled || head->current.scale != head->desired.scale) {
				log_(t, "    scale:    %.3f%s",
						wl_fixed_to_double(head->desired.scale),
						(!head->width_mm || !head->height_mm) ? " (default, size not specified)" : ""
					);
			}
			if (!head->current.enabled || head->current.x != head->desired.x || head->current.y != head->desired.y) {
				log_(t, "    position: %d,%d",
						head->desired.x,
						head->desired.y
					);
			}
		}
		if (!head->current.enabled) {
			log_(t, "    (enabled)");
		}
	} else {
		log_(t, "    (disabled)");
	}
}

void print_head(enum LogThreshold t, enum InfoEvent event, struct Head *head) {
	if (!head)
		return;

	switch (event) {
		case ARRIVED:
		case NONE:
			log_(t, "\n%s%s:", head->name, event == ARRIVED ? " Arrived" : "");
			log_(t, "  info:");
			if (head->name)
				log_(t, "    name:     '%s'", head->name);
			if (head->make)
				log_(t, "    make:     '%s'", head->make);
			if (head->model)
				log_(t, "    model:    '%s'", head->model);
			if (head->serial_number)
				log_(t, "    serial:   '%s'", head->serial_number);
			if (head->description)
				log_(t, "    desc:     '%s'", head->description);
			if (head->width_mm && head->height_mm) {
				log_(t, "    width:    %dmm", head->width_mm);
				log_(t, "    height:   %dmm", head->height_mm);
				if (head->preferred_mode) {
					log_(t, "    dpi:      %.2f @ %dx%d", mode_dpi(head->preferred_mode), head->preferred_mode->width, head->preferred_mode->height);
				}
			} else {
				log_(t, "    width:    (not specified)");
				log_(t, "    height:   (not specified)");
			}
			print_modes_res_refresh(t, head);
			print_modes_failed(t, head);
			log_(t, "  current:");
			print_head_current(t, head);
			break;
		case DEPARTED:
			log_(t, "\n%s Departed:", head->name);
			if (head->name)
				log_(t, "    name:     '%s'", head->name);
			if (head->description)
				log_(t, "    desc:     '%s'", head->description);
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

void print_heads(enum LogThreshold t, enum InfoEvent event, struct SList *heads) {
	for (struct SList *i = heads; i; i = i->nex) {
		print_head(t, event, i->val);
	}
}

