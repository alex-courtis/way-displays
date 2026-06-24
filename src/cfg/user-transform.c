#include <stdbool.h>
#include <stdlib.h>
#include <wayland-client-protocol.h>

#include "cfg/user-transform.h"

#include "fn.h"
#include "smap.h"

// TODO SMapI

static bool user_transform_equal(const struct UserTransform* const a, const struct UserTransform* const b) {
	return a && b && a->transform == b->transform;
}

struct UserTransform *cfg_user_transform_init(const enum wl_output_transform transform) {
	struct UserTransform *ut = calloc(1, sizeof(struct UserTransform));

	ut->transform = transform;

	return ut;
}

const struct SMap *user_transform_smap_init(void) {
	const struct SMapParams params = {
		.equal_val = (fn_equal)user_transform_equal,
		.clone_val = (fn_clone)user_transform_clone,
	};
	return smap_init_with(params);
}

void *user_transform_clone(const struct UserTransform* const from) {
	if (!from)
		return NULL;

	struct UserTransform *to = (struct UserTransform*)calloc(1, sizeof(struct UserTransform));

	*to = *from;

	return to;
}

