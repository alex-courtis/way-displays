#ifndef DESIRE_H
#define DESIRE_H

#include "head.h"
#include "pslist.h"
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
struct Pslist *desire_order(const struct SSet * const order_name_desc, struct Pslist *heads);
void desire_scaled_dimensions(struct Head * const head);
void desire_positions(struct Pslist *heads);

#endif // DESIRE_H

