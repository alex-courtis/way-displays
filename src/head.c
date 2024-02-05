#include <math.h>
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
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

bool head_is_max_preferred_refresh(struct Head *head) {
	if (!head)
		return false;

	for (struct SList *i = cfg->max_preferred_refresh_name_desc; i; i = i->nex) {
		if (head_matches_name_desc(head, i->val)) {
			return true;
		}
	}
	return false;
}

bool head_matches_user_mode(const void *user_mode, const void *head) {
	return user_mode && head && head_matches_name_desc((struct Head*)head, ((struct UserMode*)user_mode)->name_desc);
}

struct Mode *max_mode(struct Head *head) {
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

bool head_matches_name_desc_regex(const void *h, const void *n) {
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

bool head_matches_name_desc_fuzzy(const void *h, const void *n) {
	const struct Head *head = h;
	const char *name_desc = n;

	if (!name_desc || !head || name_desc[0] == '!')
		return false;

	return (
			(head->name && strcasestr(head->name, name_desc)) ||
			(head->description && strcasestr(head->description, name_desc))
		   );
}

bool head_matches_name_desc(const void *h, const void *n) {
	return head_matches_name_desc_exact(h, n) ||
		head_matches_name_desc_regex(h, n) ||
		head_matches_name_desc_fuzzy(h, n);
}

bool head_name_desc_matches_head(const void *n, const void *h) {
	return head_matches_name_desc(h, n);
}

bool head_matches_name_desc_exact(const void *h, const void *n) {
	const struct Head *head = h;
	const char *name_desc = n;

	if (!name_desc || !head)
		return false;

	return (head->name && strcmp(head->name, name_desc) == 0) ||
		(head->description && strcmp(head->description, name_desc) == 0);
}

wl_fixed_t head_get_fixed_scale(const struct Head *head, double scale, int32_t base) {
	// computes a scale value that is appropriate for putting into `zwlr_output_configuration_head_v1_set_scale`

	wl_fixed_t fixed_scale = wl_fixed_from_double(scale);
	wl_fixed_t fixed_scale_before = fixed_scale;

	// See !138
	base = base ? base : HEAD_DEFAULT_SCALING_BASE;
	fixed_scale = round((double)fixed_scale / HEAD_WLFIXED_SCALING_BASE * base) \
		* ((double)HEAD_WLFIXED_SCALING_BASE / base);
	if (fixed_scale != fixed_scale_before) {
		log_debug("\n%s: Rounded scale %g to nearest multiple of 1/%d: %.03f", head && head->name ? head->name : "???", scale, base, wl_fixed_to_double(fixed_scale));
	}

	return fixed_scale;
}

int32_t head_get_scaled_length(int32_t length, wl_fixed_t fixed_scale, int32_t base) {
	// scales a (pixel) length by fixed_scale

	// in case `base` comes from a not fully initialized Head (like in tests)
	base = base ? base : HEAD_DEFAULT_SCALING_BASE;

	fixed_scale = (double)fixed_scale / HEAD_WLFIXED_SCALING_BASE * base + 0.5;

	// wayland truncates when calculating size
	return floor((double)length * base / fixed_scale);
}

wl_fixed_t head_auto_scale(struct Head *head, double min, double max) {
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

void head_scaled_dimensions(struct Head *head) {
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

struct Mode *head_find_mode(struct Head *head) {
	if (!head)
		return NULL;

	if (slist_length(head->modes) == slist_length(head->modes_failed)) {
		return NULL;
	}

	static char buf[512];

	struct Mode *mode = NULL;

	// maybe a user mode
	struct UserMode *um = slist_find_equal_val(cfg->user_modes, head_matches_user_mode, head);
	if (um) {
		mode = mode_user_mode(head->modes, head->modes_failed, um);
		if (!mode && !um->warned_no_mode) {
			um->warned_no_mode = true;
			info_user_mode_string(um, buf, sizeof(buf));
			log_warn("\n%s: No available mode for %s, falling back to preferred", head->name, buf);
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
			log_info("\n%s: No preferred mode, falling back to maximum available", head->name);
		}
	}

	// last chance maximum
	if (!mode) {
		mode = max_mode(head);
	}

	return mode;
}

struct Mode *head_preferred_mode(struct Head *head) {
	if (!head)
		return NULL;

	for (struct SList *i = head->modes; i; i = i->nex) {
		if (((struct Mode*)i->val)->preferred) {
			return i->val;
		}
	}

	return NULL;
}

bool head_current_not_desired(const void *data) {
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

bool head_current_mode_not_desired(const void *data) {
	const struct Head *head = data;

	return (head && head->desired.mode != head->current.mode);
}

bool head_current_adaptive_sync_not_desired(const void *data) {
	const struct Head *head = data;

	return (head && head->desired.adaptive_sync != head->current.adaptive_sync);
}

void head_free(void *data) {
	struct Head *head = data;

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

void head_release_mode(struct Head *head, struct Mode *mode) {
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

void heads_release_head(struct Head *head) {
	slist_remove_all(&heads_arrived, NULL, head);
	slist_remove_all(&heads_departed, NULL, head);
	slist_remove_all(&heads, NULL, head);
}

void heads_destroy(void) {

	slist_free_vals(&heads, head_free);
	slist_free_vals(&heads_departed, head_free);

	slist_free(&heads_arrived);
}

