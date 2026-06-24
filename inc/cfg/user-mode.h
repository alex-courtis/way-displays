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

struct UserMode *user_mode_init(const bool max, const int32_t width, const int32_t height, const int32_t refresh_mhz, const bool warned_no_mode);

struct UserMode *user_mode_init_default(void);

const struct SMap *user_mode_smap_init(void);

struct UserMode *user_mode_clone(const struct UserMode * const from);

bool user_mode_invalid(const char* const name_desc, const struct UserMode* const user_mode, const void* const data);

#endif // CFG_USER_MODE_H
