#ifndef LAYOUT_H
#define LAYOUT_H

#include <stdbool.h>

// final loop driver function, may take action following wlr callbacks
void layout(void);

// incrementally apply any deltas between head.desired and head.current: enabled, mode, vrr, remainder
void apply(void);

// zwlr_output_configuration_v1.succeeded
void handle_success(void);

// zwlr_output_configuration_v1.failed
void handle_failure(void);

// zwlr_output_configuration_v1.cancelled
bool handle_cancelled(void);

#endif // LAYOUT_H

