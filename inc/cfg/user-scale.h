#ifndef CFG_USER_SCALE_H
#define CFG_USER_SCALE_H

#include <stdbool.h>

struct UserScale {
	float scale;
};

struct UserScale *user_scale_init(const float scale);

const struct SMap *user_scale_smap_init(void);

struct UserScale *user_scale_clone(const struct UserScale * const from);

bool user_scale_invalid(const char* const name_desc, const struct UserScale* const user_scale, const void* const data);

void user_scale_free(struct UserScale *user_scale);

#endif // CFG_USER_SCALE_H
