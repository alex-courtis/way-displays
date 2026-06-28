#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "cfg/user-mode.h"

#include "fn.h"
#include "log.h"
#include "mode.h"
#include "smap.h"

struct UserMode *user_mode_init(const bool max, const int32_t width, const int32_t height, const int32_t refresh_mhz, const bool warned_no_mode) {
	struct UserMode *um = (struct UserMode*)calloc(1, sizeof(struct UserMode));

	um->max = max;
	um->width = width;
	um->height = height;
	um->refresh_mhz = refresh_mhz;
	um->warned_no_mode = warned_no_mode;

	return um;
}

struct UserMode *user_mode_init_default(void) {
	return user_mode_init(false, -1, -1, -1, false);
}

const struct SMap *user_mode_smap_init(void) {
	const struct SMapParams params = {
		.equal_val = (fn_equal)user_mode_equal,
		.clone_val = (fn_clone)user_mode_clone,
		.free_val = (fn_free)user_mode_free,
	};
	return smap_init_with(params);
}

struct UserMode *user_mode_clone(const struct UserMode * const from) {
	if (!from)
		return NULL;

	struct UserMode *to = (struct UserMode*)calloc(1, sizeof(struct UserMode));

	*to = *from;

	return to;
}

// TODO testing mode.c does its own equality - converge them
bool user_mode_equal(const struct UserMode* const a, const struct UserMode* const b) {
	if (!a || !b) {
		return false;
	}

	if (a->max != b->max) {
		return false;
	}

	if (a->width != b->width || a->height != b->height) {
		return false;
	}

	if ((a->refresh_mhz != -1 || b->refresh_mhz != -1) && a->refresh_mhz != b->refresh_mhz) {
		return false;
	}

	return true;
}

void user_mode_free(struct UserMode *user_mode) {
	free(user_mode);
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

