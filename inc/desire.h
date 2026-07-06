#ifndef DESIRE_H
#define DESIRE_H

#include "head.h"
#include "slist.h"
#include "sset.h"

// set head.desired to head.current then invoke all desire_ in order
void desire(void);

// set head.desired if changes are needed
void desire_enabled(struct Head *head);
void desire_mode(struct Head *head);
void desire_scale(struct Head *head);
void desire_transform(struct Head *head);
void desire_adaptive_sync(struct Head *head);
void desire_reapply(struct Head *head);
struct SList *desire_order(const struct SSet * const order_name_desc, struct SList *heads);
void desire_positions(struct SList *heads);

#endif // DESIRE_H

