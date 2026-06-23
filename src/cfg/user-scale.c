#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cfg/user-scale.h"

#include "smap.h"

bool user_scale_equal(const void *a, const void *b) {
	if (!a || !b) {
		return false;
	}

	const struct UserScale *lhs = (struct UserScale*)a;
	const struct UserScale *rhs = (struct UserScale*)b;

	if (!lhs->name_desc || !rhs->name_desc) {
		return false;
	}

	return strcmp(lhs->name_desc, rhs->name_desc) == 0 && lhs->scale == rhs->scale;
}

struct UserScale *user_scale_init(const char *name_desc, const float scale) {
	struct UserScale *us = calloc(1, sizeof(struct UserScale));

	us->name_desc = strdup(name_desc);
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

void* user_scale_clone(const void* const val) {
	const struct UserScale *original = (struct UserScale*)val;
	struct UserScale *clone = (struct UserScale*)calloc(1, sizeof(struct UserScale));

	*clone = *original;
	clone->name_desc = strdup(original->name_desc);

	return clone;
}

void user_scale_free(const void *val) {
	struct UserScale *user_scale = (struct UserScale*)val;

	if (!user_scale)
		return;

	free(user_scale->name_desc);

	free(user_scale);
}

