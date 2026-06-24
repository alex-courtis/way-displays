#include <stdbool.h>
#include <stdlib.h>

#include "cfg/user-scale.h"

#include "fn.h"
#include "log.h"
#include "smap.h"

// TODO SMapF or SMapI

static bool user_scale_equal(const struct UserScale* const a, const struct UserScale* const b) {
	return a && b && a->scale == b->scale;
}

struct UserScale *user_scale_init(const float scale) {
	struct UserScale *us = calloc(1, sizeof(struct UserScale));

	us->scale = scale;

	return us;
}

const struct SMap *user_scale_smap_init(void) {
	const struct SMapParams params = {
		.equal_val = (fn_equal)user_scale_equal,
		.clone_val = (fn_clone)user_scale_clone,
	};
	return smap_init_with(params);
}

struct UserScale *user_scale_clone(const struct UserScale * const from) {
	if (!from)
		return NULL;

	struct UserScale *to = (struct UserScale*)calloc(1, sizeof(struct UserScale));

	*to = *from;

	return to;
}

bool user_scale_invalid(const char* const name_desc, const struct UserScale* const user_scale, const void* const data) {
	if (!name_desc || !user_scale) {
		return true;
	}

	if (user_scale->scale <= 0) {
		log_warn(NULL);
		log_warn("Ignoring non-positive SCALE %s %.3f", name_desc, user_scale->scale);
		return true;
	}

	return false;
}

