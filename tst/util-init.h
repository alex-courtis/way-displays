#ifndef UTIL_INIT_H
#define UTIL_INIT_H

#include <stdint.h>

#include "head.h"

struct Head *head_init_name(const char *name);

struct Head *head_init_description(const char *description);

struct Disabled *disabled_init_name_desc(const char *name_desc);

struct Mode *mode_init_whr(int32_t width, int32_t height, int32_t refresh_mhz);

struct Mode *mode_init_h_whr(struct Head* const head, int32_t width, int32_t height, int32_t refresh_mhz);

struct Mode *mode_init_whr_max(int32_t width, int32_t height, int32_t refresh_mhz);

#endif // UTIL_INIT_H
