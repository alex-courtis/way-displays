#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

#include "types.h"

struct Mode *optimal_mode(struct SList *modes);

wl_fixed_t auto_scale(struct Head *head);

void order_enable_heads(struct SList *order_name_desc, struct SList *heads, struct SList **heads_enabled, struct SList **heads_disabled);

#endif // UTIL_H

