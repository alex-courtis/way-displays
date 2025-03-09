#ifndef LAYOUT_H
#define LAYOUT_H

#include "head.h"
#include "slist.h"

void layout(void);

//
// visible for testing
//
struct SList *order_heads(struct SList *order_name_desc, struct SList *heads);
void position_heads(struct SList *heads);
void desire_enabled(struct Head *head);
void desire_mode(struct Head *head);
void desire_scale(struct Head *head);
void desire_transform(struct Head *head);
void desire_adaptive_sync(struct Head *head);
void handle_success(void);
void handle_failure(void);

#endif // LAYOUT_H

