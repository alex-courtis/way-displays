#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <wayland-util.h>

#include "head.h"

#include "cfg.h"
#include "info.h"
#include "list.h"
#include "log.h"
#include "mode.h"
#include "server.h"

struct SList *heads = NULL;
struct SList *heads_arrived = NULL;
struct SList *heads_departed = NULL;

bool head_is_max_preferred_refresh(struct Head *head) {
	if (!head)
		return false;

	for (struct SList *i = cfg->max_preferred_refresh_name_desc; i; i = i->nex) {
		if (head_matches_name_desc(i->val, head)) {
			return true;
		}
	}
	return false;
}

bool head_matches_user_mode(const void *user_mode, const void *head) {
	return user_mode && head && head_matches_name_desc(((struct UserMode*)user_mode)->name_desc, (struct Head*)head);
}

bool head_matches_user_transform(const void *user_transform, const void *head) {
	return user_transform && head && head_matches_name_desc(((struct UserTransform*)user_transform)->name_desc, (struct Head*)head);
}

struct Mode *user_mode(struct Head *head, struct UserMode *user_mode) {
	if (!head || !head->name || !user_mode)
		return NULL;

	struct SList *i, *j;

	// highest mode matching the user mode
	struct SList *mrrs = modes_res_refresh(head->modes);
	for (i = mrrs; i; i = i->nex) {
		struct ModesResRefresh *mrr = i->val;
		if (mrr && mrr_satisfies_user_mode(mrr, user_mode)) {
			for (j = mrr->modes; j; j = j->nex) {
				struct Mode *mode = j->val;
				if (!slist_find_equal(head->modes_failed, NULL, mode)) {
					slist_free_vals(&mrrs, mode_res_refresh_free);
					return mode;
				}
			}
		}
	}
	slist_free_vals(&mrrs, mode_res_refresh_free);

	return NULL;
}

struct Mode *preferred_mode(struct Head *head) {
	if (!head)
		return NULL;

	struct Mode *mode = NULL;
	for (struct SList *i = head->modes; i; i = i->nex) {
		if (!i->val)
			continue;
		mode = i->val;

		if (mode->preferred && !slist_find_equal(head->modes_failed, NULL, mode)) {
			return mode;
		}
	}

	return NULL;
}

struct Mode *max_preferred_mode(struct Head *head) {
	struct Mode *preferred = preferred_mode(head);

	if (!preferred)
		return NULL;

	struct Mode *mode = NULL, *max = NULL;

	for (struct SList *i = head->modes; i; i = i->nex) {
		if (!i->val)
			continue;
		mode = i->val;

		if (slist_find_equal(head->modes_failed, NULL, mode)) {
			continue;
		}

		if (mode->width != preferred->width || mode->height != preferred->height) {
			continue;
		}

		if (!max) {
			max = mode;
		} else if (mode->refresh_mhz > max->refresh_mhz) {
			max = mode;
		}
	}

	return max;
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

bool head_matches_name_desc(const void *a, const void *b) {
	const char *name_desc = a;
	const struct Head *head = b;

	if (!name_desc || !head)
		return false;

	return (
			(head->name && strcasecmp(name_desc, head->name) == 0) ||
			(head->description && strcasestr(head->description, name_desc))
		   );
}

wl_fixed_t head_auto_scale(struct Head *head) {
	if (!head || !head->desired.mode) {
		return wl_fixed_from_int(1);
	}

	// average dpi
	double dpi = mode_dpi(head->desired.mode);
	if (dpi == 0) {
		return wl_fixed_from_int(1);
	}

	// round the dpi to the nearest 12, so that we get a nice even wl_fixed_t
	long dpi_quantized = (long)(dpi / 12 + 0.5) * 12;

	// 96dpi approximately correct for older monitors and became the convention for 1:1 scaling
	return 256 * dpi_quantized / 96;
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

	head->scaled.height = (int32_t)((double)head->scaled.height * 256 / head->desired.scale + 0.5);
	head->scaled.width = (int32_t)((double)head->scaled.width * 256 / head->desired.scale + 0.5);
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
		mode = user_mode(head, um);
		if (!mode && !um->warned_no_mode) {
			um->warned_no_mode = true;
			info_user_mode_string(um, buf, sizeof(buf));
			log_warn("\n%s: No available mode for %s, falling back to preferred", head->name, buf);
		}
	}

	// always preferred
	if (!mode) {
		if (head_is_max_preferred_refresh(head)) {
			mode = max_preferred_mode(head);
		} else {
			mode = preferred_mode(head);
		}
		if (!mode && !head->warned_no_preferred) {
			head->warned_no_preferred = true;
			log_info("\n%s: No preferred mode, falling back to maximum available", head->name);
		}
	}

	// last change maximum
	if (!mode) {
		mode = max_mode(head);
	}

	return mode;
}

enum wl_output_transform head_find_transform(struct Head *head){
	if (!head)
		return WL_OUTPUT_TRANSFORM_NORMAL;
	enum wl_output_transform transform;

	// User transform value from config file
	struct UserTransform *ut = slist_find_equal_val(cfg->user_transform, head_matches_user_transform, head);

	if (ut) {
		transform = ut->transform;
	} else {
		transform = WL_OUTPUT_TRANSFORM_NORMAL;
	}
	return transform;
}

bool head_current_not_desired(const void *data) {
	const struct Head *head = data;

	return (head &&
			(head->desired.mode != head->current.mode ||
			 head->desired.scale != head->current.scale ||
			 head->desired.enabled != head->current.enabled ||
			 head->desired.x != head->current.x ||
			 head->desired.y != head->current.y ||
			 head->desired.transform != head->current.transform));
}

bool head_current_mode_not_desired(const void *data) {
	const struct Head *head = data;

	return (head && head->desired.mode != head->current.mode);
}

void head_free(void *data) {
	struct Head *head = data;

	if (!head)
		return;

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

