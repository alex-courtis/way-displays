#ifndef CALC_H
#define CALC_H

#include "types.h"

struct Mode *optimal_mode(struct SList *modes);

wl_fixed_t auto_scale(struct Head *head);

struct SList *order_heads(struct SList *order_name_desc, struct SList *heads);

void ltr_heads(struct SList *heads);

#endif // CALC_H

