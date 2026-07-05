#include <math.h>
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-util.h>

#include "head.h"

#include "cfg.h"
#include "cfg/disabled.h"
#include "fn.h"
#include "info.h"
#include "log.h"
#include "mode.h"
#include "pset.h"
#include "slist.h"
#include "smap.h"
#include "sset.h"
#include "str.h"
#include "wlr-output-management-unstable-v1.h"

struct SList *g_heads = NULL;
struct SList *g_heads_arrived = NULL;
struct SList *g_heads_departed = NULL;

static int32_t head_desired_scaled_length(const struct Head * const head, const int32_t length) {
	// scales a (pixel) length by fixed_scale

	int32_t b = g_cfg->scale_round_to ? g_cfg->scale_round_to : SCALE_ROUND_TO_DEFAULT;

	wl_fixed_t f = (double)head->desired.scale / 256 * b + 0.5;

	// wayland truncates when calculating size
	return floor((double)length * b / f);
}

struct Head *head_init(void) {
	struct Head *head = calloc(1, sizeof(struct Head));

	head->modes = mode_pset_init();
	head->modes_failed = mode_pset_init();

	return head;
}

struct Head *head_introduce(struct zwlr_output_head_v1 *zwlr_head) {
	if (!zwlr_head)
		return NULL;

	struct Head *head = head_init();
	head->zwlr_head = zwlr_head;

	slist_append(&g_heads, head);
	slist_append(&g_heads_arrived, head);

	return head;
}

void head_free(struct Head *head) {
	if (!head)
		return;

	if (head->current.mode && !pset_contains(head->modes, head->current.mode) && !pset_contains(head->modes_failed, head->current.mode)) {
		mode_free((struct Mode*)head->current.mode);
		if (head->desired.mode == head->current.mode) {
			head->desired.mode = NULL;
		}
	}
	if (head->desired.mode && !pset_contains(head->modes, head->desired.mode) && !pset_contains(head->modes_failed, head->desired.mode)) {
		mode_free((struct Mode*)head->desired.mode);
	}

	pset_free_vals(head->modes);
	pset_free_vals(head->modes_failed);

	free(head->name);
	free(head->description);
	free(head->make);
	free(head->model);
	free(head->serial_number);

	free(head);
}

void head_release(struct Head * const head) {
	if (!head)
		return;

	// dummy Head, just for printing
	struct Head *head_departed = head_init();
	head_departed->name = strdup(head->name);
	head_departed->description = strdup(head->description);
	slist_append(&g_heads_departed, head_departed);

	slist_remove_all(&g_heads_arrived, NULL, head);
	slist_remove_all(&g_heads_departed, NULL, head);
	slist_remove_all(&g_heads, NULL, head);

	head_free(head);
}

void head_release_mode(struct Mode *mode) {
	if (!mode)
		return;

	struct Head *head = mode->head;

	if (head) {
		if (head->desired.mode == mode) {
			head->desired.mode = NULL;
		}
		if (head->current.mode == mode) {
			head->current.mode = NULL;
		}

		if (!pset_remove_free(head->modes, mode)) {
			mode_free(mode);
		}
	} else {
		mode_free(mode);
	}
}

void heads_destroy(void) {

	slist_free_vals(&g_heads, (fn_free)head_free);
	slist_free_vals(&g_heads_departed, (fn_free)head_free);

	slist_free(&g_heads_arrived);
}

void head_set_scaled_dimensions(struct Head * const head) {
	if (!head || !head->desired.mode || !head->desired.scale) {
		return;
	}

	if (head->desired.transform % 2 == 0) {
		head->scaled.width = head->desired.mode->width;
		head->scaled.height = head->desired.mode->height;
	} else {
		head->scaled.width = head->desired.mode->height;
		head->scaled.height = head->desired.mode->width;
	}

	head->scaled.height = head_desired_scaled_length(head, head->scaled.height);
	head->scaled.width = head_desired_scaled_length(head, head->scaled.width);
}

void head_apply_toggles(struct Head * const head, const struct Cfg* cfg) {
	if (pset_match(cfg->disableds, (fn_match_ptr)disabled_name_desc_matches_head, head)) {
		if (head->overrided_enabled == NoOverride) {
			log_info(NULL);
			log_info("Applying \"DISABLED\" override for %s", head->name);
			if (head->current.enabled) {
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

struct Mode *head_add_mode(struct Head * const head, struct zwlr_output_mode_v1 *zwlr_mode) {
	if (!head || !zwlr_mode)
		return NULL;

	struct Mode *mode = mode_init();
	mode->head = head;
	mode->zwlr_mode = zwlr_mode;

	pset_add(head->modes, mode);

	return mode;
}

void head_set_current_mode(struct Head * const head, const struct zwlr_output_mode_v1 *zwlr_mode) {
	if (!head || !zwlr_mode)
		return;

	const struct Mode *mode = pset_match(head->modes, (fn_match_ptr)mode_is_zwlr_mode, zwlr_mode);

	if (mode) {
		head->current.mode = mode;
	}
}

void head_set_mode_preferred(const struct Mode * const mode) {
	if (!mode || !mode->head)
		return;

	struct Head *head = mode->head;

	if (head->mode_preferred && head->mode_preferred != mode) {

		char *existing_str = mode_str(head->mode_preferred);
		char *new_str = mode_str(mode);

		if (head->name) {
			log_info(NULL);
			log_info("%s: multiple preferred modes advertised: using initial %s, ignoring %s", head->name, existing_str, new_str);
		} else {
			log_info(NULL);
			log_info("???: multiple preferred modes advertised: using initial %s, ignoring %s", existing_str, new_str);
		}

		free(existing_str);
		free(new_str);

		return;
	}

	// set new preferred
	mode->head->mode_preferred = mode;
}

void heads_reapply(struct SList *heads) {
	log_info(NULL);
	log_info("Reapply:");

	for (struct SList *i = heads; i; i = i->nex) {
		struct Head *head = (struct Head*)i->val;

		int step = 1;

		log_info("  %s:", head->name);
		log_info("    %d: Clear current mode", step++);
		log_info("    %d: Disable", step++);

		if (pset_size(head->modes_failed) > 0) {
			log_info("    %d: Clear failed modes:", step++);

			for (const struct PSetIt *it = pset_it(head->modes_failed); it; it = pset_it_next(it)) {

				// add all failed back to modes
				pset_add(head->modes, it->val);

				char *str = mode_str(it->val);
				log_info("      %s", str);
				free(str);
			}

			// clear failed
			pset_free(head->modes_failed);
			head->modes_failed = mode_pset_init();
		}

		if (head->current.enabled) {
			char *str = mode_str(head->current.mode);
			log_info("    %d: Enable with mode:", step++);
			log_info("      %s", str);
			free(str);
		} else {
			log_info("    %d: Enable according to config", step++);
		}

		head->reapply_required = true;
		head->current.mode = NULL;
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

bool head_current_not_desired(const struct Head * const head) {
	return (head &&
			(head->desired.mode != head->current.mode ||
			 head->desired.scale != head->current.scale ||
			 head->desired.enabled != head->current.enabled ||
			 head->desired.x != head->current.x ||
			 head->desired.y != head->current.y ||
			 head->desired.transform != head->current.transform ||
			 head->desired.adaptive_sync != head->current.adaptive_sync));
}

bool head_current_mode_not_desired(const struct Head * const head) {
	return (head && head->desired.mode != head->current.mode);
}

bool head_current_adaptive_sync_not_desired(const struct Head * const head) {
	return (head && head->desired.adaptive_sync != head->current.adaptive_sync);
}

bool head_reapply_required(const struct Head * const head) {
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

	if (!head->desired.mode) {
		return head_get_fixed_scale(1.0);
	}

	// average dpi
	double dpi = mode_dpi(head->desired.mode);
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

size_t head_num_current_not_desired(struct SList * const heads) {
	size_t n = 0;

	struct SList *i = heads;
	while ((i = slist_find(i, (fn_test)head_current_not_desired))) {
		i = i->nex;
		n++;
	}

	return n;
}

const struct Mode *head_find_mode(struct Head * const head) {
	if (!head)
		return NULL;

	if (pset_size(head->modes) == 0) {
		log_error(NULL);
		log_error("No mode for %s, disabling.", head->name);
		call_back(ERROR, head_human(head), "\n  No mode, disabling");
		return NULL;
	}

	const struct Mode *mode = NULL;

	// maybe a cfg mode
	struct Mode *mode_cfg = (struct Mode*)smap_match_key(g_cfg->modes, (fn_match_str)head_name_desc_matches_head, head).val;
	if (mode_cfg) {
		mode = mode_best_satisfying(mode_cfg, head->modes);
		if (!mode && !mode_cfg->warned_no_mode) {
			mode_cfg->warned_no_mode = true;

			char *um = mode_str_brief(mode_cfg);

			log_warn(NULL);
			log_warn("%s: No available mode for user MODE %s, falling back to preferred", head->name, um);

			char *human = sprintf_alloc("%s\n  No available mode for user MODE %s, falling back to preferred", head_human(head), um);

			call_back(WARNING, human, NULL);

			free(um);
			free(human);
		}
	}

	// always try preferred
	if (!mode) {
		if (sset_match(g_cfg->max_preferred_refresh, (fn_match_str)head_name_desc_matches_head, head)) {
			mode = mode_max_refresh(head->mode_preferred, head->modes);
		} else {
			mode = head->mode_preferred;
		}
		if (!mode && !head->warned_no_preferred) {
			head->warned_no_preferred = true;

			log_info(NULL);
			log_info("%s: No preferred mode, falling back to maximum available", head_human(head));

			char *human = sprintf_alloc("%s\n  No preferred mode, falling back to maximum available", head_human(head));

			call_back(WARNING, human, NULL);

			free(human);
		}
	}

	// last chance maximum
	if (!mode) {
		mode = head_max_mode(head);
	}

	if (!mode) {
		log_error(NULL);
		log_error("No mode for %s, disabling.", head_human(head));
		call_back(ERROR, head_human(head), "\n  No mode, disabling");
	}

	return mode;
}

const struct Mode *head_max_mode(const struct Head * const head) {
	if (!head)
		return NULL;

	const struct Mode *mode_max = NULL;

	for (const struct PSetIt *it = pset_it(head->modes); it; it = pset_it_next(it)) {
		const struct Mode *mode = it->val;

		if (!mode_max) {
			mode_max = mode;
			continue;
		}

		// highest resolution
		if (mode->width * mode->height > mode_max->width * mode_max->height) {
			mode_max = mode;
			continue;
		}

		// highest refresh at highest resolution
		if (mode->width == mode_max->width &&
				mode->height == mode_max->height &&
				mode->refresh_mhz > mode_max->refresh_mhz) {
			mode_max = mode;
			continue;
		}
	}

	return mode_max;
}
