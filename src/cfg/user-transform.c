#include <stdbool.h>
#include <stdlib.h>
#include <wayland-client-protocol.h>

#include "cfg/user-transform.h"

#include "smap.h"

// TODO SMapI

// TODO type
static bool user_scale_equal(const void *a, const void *b) {
	if (!a || !b) {
		return false;
	}

	const struct UserTransform *lhs = (struct UserTransform*)a;
	const struct UserTransform *rhs = (struct UserTransform*)b;

	if (lhs->transform != rhs->transform) {
		return false;
	}

	return true;
}

struct UserTransform *cfg_user_transform_init(const enum wl_output_transform transform) {
	struct UserTransform *ut = calloc(1, sizeof(struct UserTransform));

	ut->transform = transform;

	return ut;
}

const struct SMap *user_transform_smap_init(void) {
	const struct SMapParams params = {
		.equal_val = user_scale_equal,
		.free_val = cfg_user_transform_free,
		.clone_val = fn_clone_cfg_user_transform,
	};
	return smap_init_with(params);
}

void *fn_clone_cfg_user_transform(const void* const val) {
	const struct UserTransform *original = (struct UserTransform*)val;
	struct UserTransform *clone = (struct UserTransform*)calloc(1, sizeof(struct UserTransform));

	*clone = *original;

	return clone;
}

void cfg_user_transform_free(const void *val) {
	struct UserTransform *user_transform = (struct UserTransform*)val;

	if (!user_transform)
		return;

	free(user_transform);
}

