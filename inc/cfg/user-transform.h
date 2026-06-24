#ifndef CFG_USER_TRANSFORM_H
#define CFG_USER_TRANSFORM_H

#include <wayland-client-protocol.h>

struct UserTransform {
	enum wl_output_transform transform;
};

struct UserTransform *cfg_user_transform_init(const enum wl_output_transform transform);

const struct SMap *user_transform_smap_init(void);

void *user_transform_clone(const struct UserTransform* const from);

#endif // CFG_USER_TRANSFORM_H
