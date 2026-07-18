#include <math.h>
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-util.h>

#include "head.h"

#include "cfg/cfg.h"
#include "cfg/disabled.h"
#include "enum.h"
#include "fn.h"
#include "info/callback.h"
#include "log.h"
#include "mode.h"
#include "ppmap.h"
#include "pset.h"
#include "spmap.h"
#include "sset.h"
#include "str.h"
#include "wlr-output-management-unstable-v1.h"

static char *head_str(const struct Head *head) {
	return sprintf_alloc(
			".name = \"%s\", .description = \"%s\", ",
			head->name,
			head->description
			);
}

struct Head *head_init(void) {
	struct Head *head = calloc(1, sizeof(struct Head));

	head->modes = mode_ppmap_init();
	head->modes_failed = mode_ppmap_init();

	return head;
}

// dummy head for departure printing
struct Head *head_dummy_init(const struct Head * const head) {
	struct Head *dummy = head_init();

	dummy->name = strdup(head->name ? head->name : "???");
	dummy->description = strdup(head->description ? head->description : "???");

	return dummy;
}

const struct Pset *head_pset_init(void) {
	const struct PsetParams params = {
		.str_val = (fn_str)head_str,
		.free_val = (fn_free)head_free,
	};
	return pset_init_with(params);
}

const struct PPmap *head_ppmap_init(void) {
	const struct PPmapParams params = {
		.str_val = (fn_str)head_str,
		.free_val = (fn_free)head_free,
	};
	return ppmap_init_with(params);
}

void head_free(struct Head *head) {
	if (!head)
		return;

	ppmap_free_vals(head->modes);
	ppmap_free_vals(head->modes_failed);

	free(head->name);
	free(head->description);
	free(head->make);
	free(head->model);
	free(head->serial_number);

	free(head);
}

void head_release_mode(struct Head * const head, const struct zwlr_output_mode_v1 *zmode) {
	if (!head || !zmode)
		return;

	ppmap_remove_free(head->modes, zmode);

	if (head->zmode_pref == zmode)
		head->zmode_pref = NULL;

	if (head->cur.zmode == zmode)
		head->cur.zmode = NULL;

	if (head->des.zmode == zmode)
		head->des.zmode = NULL;
}

void head_apply_toggles(struct Head * const head, const struct Cfg* cfg) {
	if (pset_find(cfg->disableds, (fn_2pred)cfg_disabled_name_desc_matches_head, head)) {
		if (head->overrided_enabled == NoOverride) {
			log_info(NULL);
			log_info("Applying \"DISABLED\" override for %s", head->name);
			if (head->cur.enabled) {
				head->overrided_enabled = OverrideFalse;
			} else {
				head->overrided_enabled = OverrideTrue;
			}
		} else {
			log_info(NULL);
			log_info("Resetting \"DISABLED\" override for %s", head->name);
			head->overrided_enabled = NoOverride;
		}
	}
}

void head_set_description(struct Head * const head, const char *description) {
	if (!head)
		return;

	if (head->description)
		free(head->description);
	head->description = NULL;

	if (description) {
		while (strstr(description, "(null) ") == description) {
			description += 7;
		}
		head->description = strdup(description);
	}
}

void head_set_mode_pref(struct Head * const head, const struct zwlr_output_mode_v1* const zmode) {
	const struct Mode *mode_cur_pref = ppmap_get(head->modes, head->zmode_pref);
	const struct Mode *mode_new_pref = ppmap_get(head->modes, zmode);

	if (mode_cur_pref && mode_new_pref && head->zmode_pref != zmode) {
		char *cur_str = mode_str_pref(mode_cur_pref, true);
		char *new_str = mode_str_pref(mode_new_pref, false);

		if (head->name) {
			log_info(NULL);
			log_info("%s: multiple preferred modes advertised: using initial %s, ignoring %s", head->name, cur_str, new_str);
		} else {
			log_info(NULL);
			log_info("???: multiple preferred modes advertised: using initial %s, ignoring %s", cur_str, new_str);
		}

		free(cur_str);
		free(new_str);

		return;
	}

	// set new preferred
	head->zmode_pref = zmode;
}

void heads_reapply(const struct PPmap *heads) {
	log_info(NULL);
	log_info("Reapply:");

	for (const struct PPmapIt *hit = ppmap_it(heads); hit; hit = ppmap_it_next(hit)) {
		struct Head *head = (struct Head*)hit->val;

		int step = 1;

		log_info("  %s:", head->name);
		log_info("    %d: Clear current mode", step++);
		log_info("    %d: Disable", step++);

		if (ppmap_size(head->modes_failed) > 0) {
			log_info("    %d: Clear failed modes:", step++);

			for (const struct PPmapIt *mit = ppmap_it(head->modes_failed); mit; mit = ppmap_it_next(mit)) {

				// add all failed back to modes
				ppmap_put(head->modes, mit->key, mit->val);

				char *str = mode_str_pref(mit->val, mit->key == head->zmode_pref);
				log_info("      %s", str);
				free(str);
			}

			// clear failed
			ppmap_free(head->modes_failed);
			head->modes_failed = mode_ppmap_init();
		}

		if (head->cur.enabled) {
			char *str = mode_str_pref(ppmap_get(head->modes, head->cur.zmode), head->cur.zmode == head->zmode_pref);
			log_info("    %d: Enable with mode:", step++);
			log_info("      %s", str);
			free(str);
		} else {
			log_info("    %d: Enable according to config", step++);
		}

		head->reapply_required = true;
		head->cur.zmode = NULL;
	}
}

const char *head_human(const struct Head * const head) {
	static const char *unknown = "???";

	if (!head) {
		return unknown;
	} else if (head->description) {
		return head->description;
	} else if (head->name) {
		return head->name;
	}

	return unknown;
}

bool head_matches_name_desc_exact(const struct Head * const head, const char * const name_desc) {
	return head && name_desc &&
		((head->name && strcmp(head->name, name_desc) == 0) ||
		 (head->description && strcmp(head->description, name_desc) == 0));
}

bool head_matches_name_desc_regex(const struct Head * const head, const char * const name_desc) {
	if (!head || !name_desc || name_desc[0] != '!')
		return false;

	const char *regex_pattern = name_desc + 1;

	regex_t regex;
	int result;

	result = regcomp(&regex, regex_pattern, REG_EXTENDED);
	if (result) {
		char error_msg[100];
		regerror(result, &regex, error_msg, sizeof(error_msg));
		log_debug("Could not compile Head NAME_DESC regex '%s': %s", regex_pattern, error_msg);
		return false;
	}

	result = REG_NOMATCH;
	if (head->name) {
		result = regexec(&regex, head->name, 0, NULL, 0);
	}
	if (result && head->description) {
		result = regexec(&regex, head->description, 0, NULL, 0);
	}
	regfree(&regex);

	return !result;
}

bool head_matches_name_desc_fuzzy(const struct Head * const head, const char * const name_desc) {
	return (name_desc && head && name_desc[0] != '!' &&
			((head->name && strcasestr(head->name, name_desc)) ||
			 (head->description && strcasestr(head->description, name_desc)))
		   );
}

bool head_matches_name_desc(const struct Head * const head, const char * const name_desc) {
	return head_matches_name_desc_exact(head, name_desc) ||
		head_matches_name_desc_regex(head, name_desc) ||
		head_matches_name_desc_fuzzy(head, name_desc);
}

bool head_name_desc_matches_head(const char * const name_desc, const struct Head * const head) {
	return head_matches_name_desc(head, name_desc);
}

bool head_current_not_desired(const struct Head * const head, const void * const unused) {
	return (head &&
			(head->reapply_required ||
			 head->des.zmode != head->cur.zmode ||
			 head->des.scale != head->cur.scale ||
			 head->des.enabled != head->cur.enabled ||
			 head->des.x != head->cur.x ||
			 head->des.y != head->cur.y ||
			 head->des.transform != head->cur.transform ||
			 head->des.adaptive_sync != head->cur.adaptive_sync));
}

bool head_current_mode_not_desired(const struct Head * const head, const void * const unused) {
	return (head && head->des.zmode != head->cur.zmode);
}

bool head_current_adaptive_sync_not_desired(const struct Head * const head, const void * const unused) {
	return (head && head->des.adaptive_sync != head->cur.adaptive_sync);
}

bool head_reapply_required(const struct Head * const head, const void * const unused) {
	return (head && head->reapply_required);
}

// wl_fixed_t, used by the wlr-output-management protocol, uses scales in multiples of 1/256.
// Meanwhile, the fractional-scale-v1 protocol deals with scales in multiples of 1/120,
// and there are observed differences in behavior between compositors, see !138.
// We force scales to be multiples of 1/8, because gcd(256, 120) = 8.
// See #138
wl_fixed_t head_get_fixed_scale(const double scale) {
	int32_t b = g_cfg->scale_round_to ? g_cfg->scale_round_to : SCALE_ROUND_TO_DEFAULT;

	double (*round_fn)(double x);

	switch (g_cfg->scale_round_strategy) {
		case DOWN:
			round_fn = floor;
			break;
		case UP:
			round_fn = ceil;
			break;
		case NEAREST:
		default:
			round_fn = round;
			break;
	}

	return round_fn((double)wl_fixed_from_double(scale) / 256 * b) * ((double)256 / b);
}

wl_fixed_t head_auto_scale(const struct Head * const head, const double min, const double max) {
	if (!head) {
		return head_get_fixed_scale(1.0);
	}

	int32_t scaling_base = g_cfg->scale_round_to ? g_cfg->scale_round_to : SCALE_ROUND_TO_DEFAULT;

	int32_t dpi_base = g_cfg->auto_scale_dpi ? g_cfg->auto_scale_dpi : AUTO_SCALE_DPI_DEFAULT;

	if (!head->des.zmode) {
		return head_get_fixed_scale(1.0);
	}

	// average dpi
	double dpi = mode_dpi(ppmap_get(head->modes, head->des.zmode), head->width_mm, head->height_mm);
	if (dpi == 0) {
		return head_get_fixed_scale(1.0);
	}

	// convert min and max to quantized dpi inside range
	long dpi_min = dpi_base / scaling_base * (long)ceil(min * scaling_base);
	long dpi_max = dpi_base / scaling_base * (long)(max * scaling_base);
	if (dpi_min < dpi_base / scaling_base) {
		dpi_min = dpi_base / scaling_base;
	}

	// clamp dpi between min and max (if set)
	double dpi_clamped = dpi;
	if (dpi_clamped < dpi_min) {
		dpi_clamped = dpi_min;
	} else if (dpi_min <= dpi_max && dpi_clamped > dpi_max) {
		dpi_clamped = dpi_max;
	}

	return head_get_fixed_scale(dpi_clamped / dpi_base);
}

const struct zwlr_output_mode_v1 *head_find_mode(struct Head * const head) {
	if (!head)
		return NULL;

	if (ppmap_size(head->modes) == 0) {
		log_error(NULL);
		log_error("No mode for %s, disabling.", head->name);
		callback(ERROR, head_human(head), "\n  No mode, disabling");
		return NULL;
	}

	const struct Mode *mode = NULL;

	// maybe a cfg mode
	struct Mode *cfg_mode = (struct Mode*)spmap_find_key(g_cfg->modes, (fn_2pred_str)head_name_desc_matches_head, head).val;
	if (cfg_mode) {
		mode = mode_best_satisfying(cfg_mode, head->modes);
		if (!mode && !cfg_mode->warned_no_mode) {
			cfg_mode->warned_no_mode = true;

			char *um = mode_str_cfg(cfg_mode);

			log_warn(NULL);
			log_warn("%s: No available mode for user MODE %s, falling back to preferred", head->name, um);

			char *human = sprintf_alloc("%s\n  No available mode for user MODE %s, falling back to preferred", head_human(head), um);

			callback(WARNING, human, NULL);

			free(um);
			free(human);
		}
	}

	// try preferred
	if (!mode) {
		const struct Mode *mode_pref = ppmap_get(head->modes, head->zmode_pref);
		if (mode_pref) {
			if (sset_find(g_cfg->max_preferred_refresh, (fn_2pred_str)head_name_desc_matches_head, head)) {
				mode = mode_max_refresh(mode_pref, head->modes);
			} else {
				mode = mode_pref;
			}
		}
		if (!mode && !head->warned_no_preferred) {
			head->warned_no_preferred = true;

			log_info(NULL);
			log_info("%s: No preferred mode, falling back to maximum available", head_human(head));

			char *human = sprintf_alloc("%s\n  No preferred mode, falling back to maximum available", head_human(head));

			callback(WARNING, human, NULL);

			free(human);
		}
	}

	// maximum; we have already checked that modes are available
	if (!mode) {
		mode = mode_max(head->modes);
	}

	return ppmap_find_val(head->modes, equal_ptr, mode).key;
}

double head_scale(const struct Head * const head, const struct zwlr_output_mode_v1 * const zmode) {
	if (!head)
		return 1;

	double dpi = mode_dpi(ppmap_get(head->modes, zmode), head->width_mm, head->height_mm);

	if (dpi == 0)
		return 1;

	return dpi / (g_cfg->auto_scale_dpi ? g_cfg->auto_scale_dpi : AUTO_SCALE_DPI_DEFAULT);
}

