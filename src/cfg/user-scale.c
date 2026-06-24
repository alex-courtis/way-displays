#include <stdbool.h>
#include <stdlib.h>

#include "cfg/user-scale.h"

#include "log.h"
#include "smap.h"

// TODO SMapF or SMapI

static bool user_scale_equal(const void *a, const void *b) {
	if (!a || !b) {
		return false;
	}

	const struct UserScale *lhs = (struct UserScale*)a;
	const struct UserScale *rhs = (struct UserScale*)b;

	return lhs->scale == rhs->scale;
}

struct UserScale *user_scale_init(const char *name_desc, const float scale) {
	struct UserScale *us = calloc(1, sizeof(struct UserScale));

	us->scale = scale;

	return us;
}

const struct SMap *user_scale_smap_init(void) {
	const struct SMapParams params = {
		.equal_val = user_scale_equal,
		.free_val = user_scale_free,
		.clone_val = user_scale_clone,
	};
	return smap_init_with(params);
}

// TODO type
void* user_scale_clone(const void* const val) {
	const struct UserScale *original = (struct UserScale*)val;
	struct UserScale *clone = (struct UserScale*)calloc(1, sizeof(struct UserScale));

	*clone = *original;

	return clone;
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

void user_scale_free(const void *val) {
	struct UserScale *user_scale = (struct UserScale*)val;

	if (!user_scale)
		return;

	free(user_scale);
}

