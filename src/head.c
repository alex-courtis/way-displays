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
#include "cfg/user-mode.h"
#include "info.h"
#include "log.h"
#include "mode.h"
#include "pset.h"
#include "slist.h"
#include "smap.h"
#include "str.h"

struct SList *g_heads = NULL;
struct SList *g_heads_arrived = NULL;
struct SList *g_heads_departed = NULL;

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

static bool head_is_max_preferred_refresh(const struct Head * const head) {
	if (!head)
		return false;

	for (const struct SList *i = g_cfg->max_preferred_refresh_name_desc; i; i = i->nex) {
		if (head_matches_name_desc(head, i->val)) {
			return true;
		}
	}
	return false;
}

static struct Mode *max_mode(const struct Head * const head) {
	if (!head)
		return NULL;

	struct Mode *mode = NULL, *max = NULL;
	for (struct SList *i = head->modes; i; i = i->nex) {
		if (!i->val)
			continue;
		mode = i->val;

		if (slist_find_equal(head->modes_failed, NULL, mode)) {
			continue;
		}

		if (!max) {
			max = mode;
			continue;
		}

		// highest resolution
		if (mode->width * mode->height > max->width * max->height) {
			max = mode;
			continue;
		}

		// highest refresh at highest resolution
		if (mode->width == max->width &&
				mode->height == max->height &&
				mode->refresh_mhz > max->refresh_mhz) {
			max = mode;
			continue;
		}
	}

	return max;
}

// TODO normalise all these matchers

bool head_matches_name_desc_regex(const void * const h, const void * const n) {
	const struct Head *head = h;
	const char *name_desc = n;

	if (name_desc[0] != '!')
		return false;

	const char *regex_pattern = name_desc + 1;

	regex_t regex;
	int result;

	result = regcomp(&regex, regex_pattern, REG_EXTENDED);
	if (result) {
		log_debug("Could not compile regex '%s'\n", regex_pattern);
		return false;
	}

	result = REG_NOMATCH;
	if (head->name) {
		result = regexec(&regex, head->name, 0, NULL, 0);
	}
	if (result && head->description) {
		result = regexec(&regex, head->description, 0, NULL, 0);
	}
	if (result && result != REG_NOMATCH) {
		char error_msg[100];
		regerror(result, &regex, error_msg, sizeof(error_msg));
		log_debug("Regex match failed: %s\n", error_msg);
	}
	regfree(&regex);

	return !result;
}

bool head_matches_name_desc_fuzzy(const void * const h, const void * const n) {
	const struct Head *head = h;
	const char *name_desc = n;

	if (!name_desc || !head || name_desc[0] == '!')
		return false;

	return (
			(head->name && strcasestr(head->name, name_desc)) ||
			(head->description && strcasestr(head->description, name_desc))
		   );
}

bool head_matches_name_desc(const void * const h, const void * const n) {
	return head_matches_name_desc_exact(h, n) ||
		head_matches_name_desc_regex(h, n) ||
		head_matches_name_desc_fuzzy(h, n);
}

static bool head_name_desc_x_matches_head(const char * const name_desc, const void * const x, const void * const head) {
	return head_matches_name_desc(head, name_desc);
}

bool head_name_desc_matches_head(const char * const name_desc, const void * const head) {
	return head_matches_name_desc(head, name_desc);
}

bool head_matches_name_desc_exact(const void * const h, const void * const n) {
	const struct Head *head = h;
	const char *name_desc = n;

	if (!name_desc || !head)
		return false;

	return (head->name && strcmp(head->name, name_desc) == 0) ||
		(head->description && strcmp(head->description, name_desc) == 0);
}

bool head_disabled_matches_head(const void * const d, const void * const h) {
	const struct Disabled *disabled_if = (struct Disabled*)d;

	if (!d)
		return false;

	return head_matches_name_desc(h, disabled_if->name_desc);
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

static int32_t head_desired_scaled_length(const struct Head * const head, const int32_t length) {
	// scales a (pixel) length by fixed_scale

	int32_t b = g_cfg->scale_round_to ? g_cfg->scale_round_to : SCALE_ROUND_TO_DEFAULT;

	wl_fixed_t f = (double)head->desired.scale / 256 * b + 0.5;

	// wayland truncates when calculating size
	return floor((double)length * b / f);
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
	if (pset_match(cfg->disableds, head_disabled_matches_head, head)) {
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

struct Mode *head_find_mode(struct Head * const head) {
	if (!head)
		return NULL;

	if (slist_length(head->modes) == slist_length(head->modes_failed)) {
		log_error(NULL);
		log_error("No mode for %s, disabling.", head->name);
		call_back(ERROR, head_human(head), "\n  No mode, disabling");
		return NULL;
	}

	struct Mode *mode = NULL;

	// maybe a user mode
	struct UserMode *user_mode = (struct UserMode*)smap_match(g_cfg->user_modes, head_name_desc_x_matches_head, head).val;
	if (user_mode) {
		mode = mode_user_mode(head->modes, head->modes_failed, user_mode);
		if (!mode && !user_mode->warned_no_mode) {
			user_mode->warned_no_mode = true;

			char *user_mode_str = info_user_mode_string(user_mode);

			log_warn(NULL);
			log_warn("%s: No available mode for user MODE %s, falling back to preferred", head->name, user_mode_str);

			char *human = sprintf_alloc("%s\n  No available mode for user MODE %s, falling back to preferred", head_human(head), user_mode_str);

			call_back(WARNING, human, NULL);

			free(user_mode_str);
			free(human);
		}
	}

	// always preferred
	if (!mode) {
		if (head_is_max_preferred_refresh(head)) {
			mode = mode_max_preferred(head->modes, head->modes_failed);
		} else {
			mode = mode_preferred(head->modes, head->modes_failed);
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
		mode = max_mode(head);
	}

	if (!mode) {
		log_error(NULL);
		log_error("No mode for %s, disabling.", head_human(head));
		call_back(ERROR, head_human(head), "\n  No mode, disabling");
	}

	return mode;
}

struct Mode *head_preferred_mode(const struct Head * const head) {
	if (!head)
		return NULL;

	for (const struct SList *i = head->modes; i; i = i->nex) {
		if (((struct Mode*)i->val)->preferred) {
			return i->val;
		}
	}

	return NULL;
}

bool head_current_not_desired(const void * const data) {
	const struct Head *head = data;

	return (head &&
			(head->desired.mode != head->current.mode ||
			 head->desired.scale != head->current.scale ||
			 head->desired.enabled != head->current.enabled ||
			 head->desired.x != head->current.x ||
			 head->desired.y != head->current.y ||
			 head->desired.transform != head->current.transform ||
			 head->desired.adaptive_sync != head->current.adaptive_sync));
}

size_t head_num_current_not_desired(struct SList * const heads) {
	size_t n = 0;

	struct SList *i = heads;
	while ((i = slist_find(i, head_current_not_desired))) {
		i = i->nex;
		n++;
	}

	return n;
}

bool head_reapply_required(const void * const data) {
	const struct Head *head = data;

	return (head && head->reapply_required);
}

bool head_current_mode_not_desired(const void * const data) {
	const struct Head *head = data;

	return (head && head->desired.mode != head->current.mode);
}

bool head_current_adaptive_sync_not_desired(const void * const data) {
	const struct Head *head = data;

	return (head && head->desired.adaptive_sync != head->current.adaptive_sync);
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

void heads_reapply(struct SList *heads) {
	log_info(NULL);
	log_info("Reapply:");

	for (struct SList *i = heads; i; i = i->nex) {
		struct Head *head = (struct Head*)i->val;

		int step = 1;

		log_info("  %s:", head->name);
		log_info("    %d: Clear current mode", step++);
		log_info("    %d: Disable", step++);

		if (head->modes_failed) {
			log_info("    %d: Clear failed modes:", step++);

			for (struct SList *j = head->modes_failed; j; j = j->nex) {
				const struct Mode *mode = (struct Mode*)j->val;

				char *mode_str = info_mode_string(mode);
				log_info("      %s", mode_str);
				free(mode_str);
			}

			slist_free(&head->modes_failed);
		}

		if (head->current.enabled) {
			char *mode_str = info_mode_string(head->current.mode);
			log_info("    %d: Enable with mode:", step++);
			log_info("      %s", mode_str);
			free(mode_str);
		} else {
			log_info("    %d: Enable according to config", step++);
		}

		head->reapply_required = true;
		head->current.mode = NULL;
	}
}

void head_free(const void * const data) {
	struct Head *head = (struct Head*)data;

	if (!head)
		return;

	if (!(slist_find_equal_val(head->modes, NULL, head->current.mode))) {
		mode_free(head->current.mode);
	}
	if (!(slist_find_equal_val(head->modes, NULL, head->desired.mode))) {
		mode_free(head->desired.mode);
	}

	slist_free(&head->modes_failed);
	slist_free_vals(&head->modes, mode_free);

	free(head->name);
	free(head->description);
	free(head->make);
	free(head->model);
	free(head->serial_number);

	free(head);
}

void head_release_mode(struct Head * const head, const struct Mode * const mode) {
	if (!head || !mode)
		return;

	if (head->desired.mode == mode) {
		head->desired.mode = NULL;
	}
	if (head->current.mode == mode) {
		head->current.mode = NULL;
	}

	slist_remove_all(&head->modes, NULL, mode);
}

void heads_release_head(const struct Head * const head) {
	slist_remove_all(&g_heads_arrived, NULL, head);
	slist_remove_all(&g_heads_departed, NULL, head);
	slist_remove_all(&g_heads, NULL, head);
}

void heads_destroy(void) {

	slist_free_vals(&g_heads, head_free);
	slist_free_vals(&g_heads_departed, head_free);

	slist_free(&g_heads_arrived);
}

