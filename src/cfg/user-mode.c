#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "cfg/user-mode.h"

#include "log.h"
#include "mode.h"
#include "smap.h"

static bool user_mode_equal(const void *a, const void *b) {
	if (!a || !b) {
		return false;
	}

	const struct UserMode *lhs = (struct UserMode*)a;
	const struct UserMode *rhs = (struct UserMode*)b;

	if (lhs->max != rhs->max) {
		return false;
	}

	if (lhs->width != rhs->width || lhs->height != rhs->height) {
		return false;
	}

	if ((lhs->refresh_mhz != -1 || rhs->refresh_mhz != -1) && lhs->refresh_mhz != rhs->refresh_mhz) {
		return false;
	}

	return true;
}

void* user_mode_clone(const void* const val) {
	const struct UserMode *original = (struct UserMode*)val;
	struct UserMode *clone = (struct UserMode*)calloc(1, sizeof(struct UserMode));

	*clone = *original;

	return clone;
}

bool user_mode_invalid(const char* const name_desc, const struct UserMode* const user_mode, const void* const data) {
	if (!user_mode || !name_desc) {
		return true;
	}
	if (user_mode->width != -1 && user_mode->width <= 0) {
		log_warn(NULL);
		log_warn("Ignoring non-positive MODE %s WIDTH %d", name_desc, user_mode->width);
		return true;
	}
	if (user_mode->height != -1 && user_mode->height <= 0) {
		log_warn(NULL);
		log_warn("Ignoring non-positive MODE %s HEIGHT %d", name_desc, user_mode->height);
		return true;
	}
	if (user_mode->refresh_mhz != -1 && user_mode->refresh_mhz <= 0) {
		log_warn(NULL);
		log_warn("Ignoring non-positive MODE %s HZ %s", name_desc, mhz_to_hz_str(user_mode->refresh_mhz));
		return true;
	}

	if (!user_mode->max) {
		if (user_mode->width == -1) {
			log_warn(NULL);
			log_warn("Ignoring invalid MODE %s missing WIDTH", name_desc);
			return true;
		}
		if (user_mode->height == -1) {
			log_warn(NULL);
			log_warn("Ignoring invalid MODE %s missing HEIGHT", name_desc);
			return true;
		}
	}

	return false;
}

struct UserMode *user_mode_init_default(void) {
	return user_mode_init(NULL, false, -1, -1, -1, false);
}

struct UserMode *user_mode_init(const char *name_desc, const bool max, const int32_t width, const int32_t height, const int32_t refresh_mhz, const bool warned_no_mode) {
	struct UserMode *um = (struct UserMode*)calloc(1, sizeof(struct UserMode));

	um->max = max;
	um->width = width;
	um->height = height;
	um->refresh_mhz = refresh_mhz;
	um->warned_no_mode = warned_no_mode;

	return um;
}

const struct SMap *user_mode_smap_init(void) {
	const struct SMapParams params = {
		.equal_val = user_mode_equal,
		.free_val = user_mode_free,
		.clone_val = user_mode_clone,
	};
	return smap_init_with(params);
}

void user_mode_free(const void *val) {
	struct UserMode *user_mode = (struct UserMode*)val;

	if (!user_mode)
		return;

	free(user_mode);
}

