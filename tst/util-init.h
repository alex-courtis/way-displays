#ifndef UTIL_INIT_H
#define UTIL_INIT_H

#include <stdint.h>

#include "head.h"

struct Head *head_n(const char *name);

struct Head *head_d(const char *description);

struct CfgDisabled *disabled_nd(const char *name_desc);

struct Mode *mode_whr(int32_t width, int32_t height, int32_t refresh_mhz);

struct Mode *mode_h(struct Head* const head);

struct Mode *mode_h_whr(struct Head* const head, int32_t width, int32_t height, int32_t refresh_mhz);

struct Mode *mode_whr_max(int32_t width, int32_t height, int32_t refresh_mhz);

#endif // UTIL_INIT_H
