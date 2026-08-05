#ifndef UTIL_INIT_H
#define UTIL_INIT_H

#include <stdbool.h>
#include <stdint.h>

struct Head *head_n(const char *name);

struct Head *head_n_en(const char *name, bool enabled);

struct Head *head_d(const char *description);

struct Output *output_n(const char *name);

struct Mode *mode_whr(int32_t width, int32_t height, int32_t refresh_mhz);

struct Mode *mode_whr_max(int32_t width, int32_t height, int32_t refresh_mhz);

struct Mode *mode_whr_max_pref(int32_t width, int32_t height, int32_t refresh_mhz);

// all defaults
struct Cfg *cfg_default(void);

// scalars only, no disabled
struct Cfg *cfg_default_scalars(void);

#endif // UTIL_INIT_H
