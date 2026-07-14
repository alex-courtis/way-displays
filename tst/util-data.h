#ifndef UTIL_DATA_H
#define UTIL_DATA_H

#include "enum.h"
#include "pslist.h"

struct Cfg *cfg_all(void);

struct IpcOperation *ipc_response(void);

// add a LogCapLine to log_cap_lines
void log_cap_line_append(enum LogThreshold threshold, const char *line, struct Pslist **log_cap_lines);

#endif // UTIL_DATA_H
