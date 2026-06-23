#ifndef CFG_USER_SCALE_H
#define CFG_USER_SCALE_H

#include <stdbool.h>

struct UserScale {
	char *name_desc;
	float scale;
};

bool user_scale_equal(const void *a, const void *b);

struct UserScale *user_scale_init(const char *name_desc, const float scale);

const struct SMap *user_scale_smap_init(void);

void* user_scale_clone(const void* const val);

void user_scale_free(const void *val);

#endif // CFG_USER_SCALE_H
