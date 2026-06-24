#ifndef CFG_USER_TRANSFORM_H
#define CFG_USER_TRANSFORM_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-client-protocol.h>

struct UserTransform {
	enum wl_output_transform transform;
};

struct UserTransform *cfg_user_transform_init(const enum wl_output_transform transform);

const struct SMap *user_transform_smap_init(void);

void *fn_clone_cfg_user_transform(const void* const val);

void cfg_user_transform_free(const void *val);

#endif // CFG_USER_TRANSFORM_H
