#ifndef CALC_H
#define CALC_H

#include <wayland-util.h>
#include "head.h"
#include "list.h"

wl_fixed_t calc_auto_scale(struct Head *head);

void calc_scaled_dimensions(struct Head *head);

struct SList *calc_head_order(struct SList *order_name_desc, struct SList *heads);

void calc_head_positions(struct SList *heads);

#endif // CALC_H

