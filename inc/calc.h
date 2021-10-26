#ifndef CALC_H
#define CALC_H

#include "types.h"

double calc_dpi(struct Mode *mode);

struct Mode *optimal_mode(struct SList *modes);

wl_fixed_t auto_scale(struct Head *head);

void calc_relative_dimensions(struct Head *head);

struct SList *order_heads(struct SList *order_name_desc, struct SList *heads);

void ltr_heads(struct SList *heads, enum LtrAlign align);

#endif // CALC_H

