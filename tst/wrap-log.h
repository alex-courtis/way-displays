#ifndef WRAP_LOG_H
#define WRAP_LOG_H

#include <stdbool.h>

#include "log.h"
#include "pslist.h"

// add a LogCapLine to test captured logs
void log_cap_line(enum LogThreshold threshold, const char *line, struct Pslist **log_cap_lines);

// print log messages, useful when debugging tests
#define LOG_PRINT false

#endif // WRAP_LOG_H
