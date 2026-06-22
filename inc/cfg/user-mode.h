#ifndef CFG_USER_MODE_H
#define CFG_USER_MODE_H

#include <stdbool.h>
#include <stdint.h>

struct UserMode {
	bool max;
	int32_t width;
	int32_t height;
	int32_t refresh_mhz;
	bool warned_no_mode;
};

struct UserMode *user_mode_init(const char *name_desc, const bool max, const int32_t width, const int32_t height, const int32_t refresh_hz, const bool warned_no_mode);

struct UserMode *user_mode_init_default(void);

const struct SMap *user_mode_smap_init(void);

void* user_mode_clone(const void* const val);

bool user_mode_invalid(const char *name_desc, const struct UserMode *user_mode);

void user_mode_free(const void *val);

#endif // CFG_USER_MODE_H
