#ifndef CALLBACK_H
#define CALLBACK_H

#include "head.h"
#include "log.h"
#include "mode.h"

#define CALLBACK_MSG_LEN 1024 * 64

// execute CALLBACK_CMD if enabled
// set CALLBACK_MSG to msg1..msg2
// set CALLBACK_LEVEL to log name
void callback(const enum LogThreshold t, const char * const msg1, const char * const msg2);

// execute CALLBACK_CMD with mode failed message, if enabled
void callback_mode_fail(const enum LogThreshold t, const struct Head * const head, const struct Mode * const mode);

// execute CALLBACK_CMD with VRR failed message, if enabled
void callback_adaptive_sync_fail(const enum LogThreshold t, const struct Head * const head);

#endif // CALLBACK_H

