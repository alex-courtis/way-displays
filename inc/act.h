#ifndef ACT_H
#define ACT_H

#include <stdbool.h>

// final loop driver function, may take action following wlr callbacks
void act(void);

// incrementally apply any deltas between head.desired and head.current: enabled, mode, vrr, remainder
void act_apply(void);

// zwlr_output_configuration_v1.succeeded
void act_handle_success(void);

// zwlr_output_configuration_v1.failed
void act_handle_failure(void);

// zwlr_output_configuration_v1.cancelled
bool act_handle_cancelled(void);

#endif // ACT_H

