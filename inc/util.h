#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

#include "types.h"

struct Mode *optimal_mode(struct SList *modes);

wl_fixed_t auto_scale(struct Head *head);

struct SList *order_heads(struct SList *order_name_desc, struct SList *heads);

#endif // UTIL_H

