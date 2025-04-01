#include <math.h>
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-util.h>

#include "head.h"

#include "cfg.h"
#include "global.h"
#include "info.h"
#include "slist.h"
#include "log.h"
#include "mode.h"

struct SList *heads = NULL;
struct SList *heads_arrived = NULL;
struct SList *heads_departed = NULL;

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

	for (struct SList *i = cfg->max_preferred_refresh_name_desc; i; i = i->nex) {
		if (head_matches_name_desc(head, i->val)) {
			return true;
		}
	}
	return false;
}

static bool head_matches_user_mode(const void * const user_mode, const void * const head) {
	return user_mode && head && head_matches_name_desc((struct Head*)head, ((struct UserMode*)user_mode)->name_desc);
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

bool head_matches_name_desc_regex(const void * const h, const void * const n) {
	const struct Head *head = h;
	const char *name_desc = n;

	if (name_desc[0] != '!')
		return false;

	const char *regex_pattern = name_desc + 1;

	regex_t regex;
	int result;
	char error_msg[100];

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

bool head_name_desc_matches_head(const void * const n, const void * const h) {
	return head_matches_name_desc(h, n);
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
	struct Disabled *disabled_if = (struct Disabled*)d;

	if (!d)
		return false;

	return head_matches_name_desc(h, disabled_if->name_desc);
}

wl_fixed_t head_get_fixed_scale(const struct Head * const head, const double scale, const int32_t base) {
	// computes a scale value that is appropriate for putting into `zwlr_output_configuration_head_v1_set_scale`

	wl_fixed_t fixed_scale = wl_fixed_from_double(scale);
	wl_fixed_t fixed_scale_before = fixed_scale;

	int32_t b = base ? base : HEAD_DEFAULT_SCALING_BASE;

	// See !138
	fixed_scale = round((double)fixed_scale / HEAD_WLFIXED_SCALING_BASE * b) \
				  * ((double)HEAD_WLFIXED_SCALING_BASE / b);
	if (fixed_scale != fixed_scale_before) {
		log_debug("\n%s: Rounded scale %g to nearest multiple of 1/%d: %.03f", head_human(head), scale, b, wl_fixed_to_double(fixed_scale));
	}

	return fixed_scale;
}

static int32_t head_get_scaled_length(const int32_t length, const wl_fixed_t fixed_scale, const int32_t base) {
	// scales a (pixel) length by fixed_scale

	// in case `base` comes from a not fully initialized Head (like in tests)
	int32_t b = base ? base : HEAD_DEFAULT_SCALING_BASE;

	wl_fixed_t f = (double)fixed_scale / HEAD_WLFIXED_SCALING_BASE * b + 0.5;

	// wayland truncates when calculating size
	return floor((double)length * b / f);
}

wl_fixed_t head_auto_scale(const struct Head * const head, const double min, const double max) {
	if (!head) {
		return head_get_fixed_scale(head, 1.0, HEAD_DEFAULT_SCALING_BASE);
	}

	if (!head->desired.mode) {
		return head_get_fixed_scale(head, 1.0, head->scaling_base);
	}

	// average dpi
	double dpi = mode_dpi(head->desired.mode);
	if (dpi == 0) {
		return head_get_fixed_scale(head, 1.0, head->scaling_base);
	}

	// round the dpi to the nearest 12, so that we get a nice even wl_fixed_t
	long dpi_quantized = (long)(dpi / 12 + 0.5) * 12;

	// convert min and max to quantized dpi
	long dpi_min = 12 * (long)ceil(min * 8);
	long dpi_max = 12 * (long)(max * 8);
	if (dpi_min < 12) {
		dpi_min = 12;
	}

	// clamp dpi between min and max (if set)
	if (dpi_quantized < dpi_min) {
		dpi_quantized = dpi_min;
	} else if (dpi_min <= dpi_max && dpi_quantized > dpi_max) {
		dpi_quantized = dpi_max;
	}

	// 96dpi approximately correct for older monitors and became the convention for 1:1 scaling
	return head_get_fixed_scale(head, (double) dpi_quantized / 96, head->scaling_base);
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

	head->scaled.height = head_get_scaled_length(head->scaled.height, head->desired.scale, head->scaling_base);
	head->scaled.width = head_get_scaled_length(head->scaled.width, head->desired.scale, head->scaling_base);
}

void head_apply_toggles(struct Head * const head, struct Cfg* cfg) {
	if (slist_find_equal(cfg->disabled, head_disabled_matches_head, head) != NULL) {
		if (head->overrided_enabled == NoOverride) {
			log_info("\nEnabling \"DISABLED\" override for %s", head->name);
			if (head->current.enabled) {
				head->overrided_enabled = OverrideFalse;
			} else {
				head->overrided_enabled = OverrideTrue;
			}
		} else {
			log_info("\nDisabling \"DISABLED\" override for %s", head->name);
			head->overrided_enabled = NoOverride;
		}
	}
}

struct Mode *head_find_mode(struct Head * const head) {
	if (!head)
		return NULL;

	static char human[CALLBACK_MSG_LEN];

	if (slist_length(head->modes) == slist_length(head->modes_failed)) {
		log_error("\nNo mode for %s, disabling.", head->name);
		snprintf(human, CALLBACK_MSG_LEN, "%s\n  No mode, disabling", head_human(head));
		call_back(ERROR, head_human(head), "\n  No mode, disabling");
		return NULL;
	}

	struct Mode *mode = NULL;

	// maybe a user mode
	struct UserMode *um = slist_find_equal_val(cfg->user_modes, head_matches_user_mode, head);
	if (um) {
		mode = mode_user_mode(head->modes, head->modes_failed, um);
		if (!mode && !um->warned_no_mode) {
			um->warned_no_mode = true;

			static char um_str[512];
			info_user_mode_string(um, um_str, sizeof(um_str));

			log_warn("\n%s: No available mode for user MODE %s, falling back to preferred", head->name, um_str);

			snprintf(human, CALLBACK_MSG_LEN, "%s\n  No available mode for user MODE %s, falling back to preferred", head_human(head), um_str);
			call_back(WARNING, human, NULL);
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

			log_info("\n%s: No preferred mode, falling back to maximum available", head_human(head));

			snprintf(human, CALLBACK_MSG_LEN, "%s\n  No preferred mode, falling back to maximum available", head_human(head));
			call_back(WARNING, human, NULL);
		}
	}

	// last chance maximum
	if (!mode) {
		mode = max_mode(head);
	}

	if (!mode) {
		log_error("\nNo mode for %s, disabling.", head_human(head));
		call_back(ERROR, head_human(head), "\n  No mode, disabling");
	}

	return mode;
}

struct Mode *head_preferred_mode(const struct Head * const head) {
	if (!head)
		return NULL;

	for (struct SList *i = head->modes; i; i = i->nex) {
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

bool head_current_mode_not_desired(const void * const data) {
	const struct Head *head = data;

	return (head && head->desired.mode != head->current.mode);
}

bool head_current_adaptive_sync_not_desired(const void * const data) {
	const struct Head *head = data;

	return (head && head->desired.adaptive_sync != head->current.adaptive_sync);
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
	slist_remove_all(&heads_arrived, NULL, head);
	slist_remove_all(&heads_departed, NULL, head);
	slist_remove_all(&heads, NULL, head);
}

void heads_destroy(void) {

	slist_free_vals(&heads, head_free);
	slist_free_vals(&heads_departed, head_free);

	slist_free(&heads_arrived);
}

