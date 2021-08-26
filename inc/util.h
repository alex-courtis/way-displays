#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

#include <wayland-util.h>

#include "types.h"

// preferred, then highest resolution with the highest refresh
// may return NULL
struct Mode *optimal_mode(struct SList *modes);

wl_fixed_t auto_scale(struct Head *head);

void order_desired_heads(struct OutputManager *output_manager);

#endif // UTIL_H

