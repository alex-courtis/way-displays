#ifndef CALLBACK_H
#define CALLBACK_H

#include "enum.h"
#include "head.h"
#include "wlr-output-management-unstable-v1.h"

#define CALLBACK_MSG_LEN 1024 * 64

// execute CALLBACK_CMD if enabled
// set CALLBACK_MSG to msg1..msg2
// set CALLBACK_LEVEL to log name
void callback(const enum LogThreshold t, const char * const msg1, const char * const msg2);

// execute CALLBACK_CMD with mode failed message, if enabled
void callback_mode_fail(const enum LogThreshold t, const struct Head * const head, const struct zwlr_output_mode_v1* const zwlr_mode);

// execute CALLBACK_CMD with VRR failed message, if enabled
void callback_adaptive_sync_fail(const enum LogThreshold t, const struct Head * const head);

#endif // CALLBACK_H

