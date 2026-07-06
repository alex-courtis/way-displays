#ifndef LAYOUT_H
#define LAYOUT_H

#include <stdbool.h>

#include "head.h"
#include "slist.h"
#include "sset.h"

// final loop driver function, may take action following wlr callbacks
void layout(void);

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

// zwlr_output_configuration_v1.succeeded
void handle_success(void);

// zwlr_output_configuration_v1.failed
void handle_failure(void);

// zwlr_output_configuration_v1.cancelled
bool handle_cancelled(void);

#endif // LAYOUT_H

