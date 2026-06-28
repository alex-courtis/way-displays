#ifndef ASSERT_LOG_H
#define ASSERT_LOG_H

#include "log.h"

void _assert_log(enum LogThreshold t, const char* s, const char * const file, const int line);
#define assert_log(t, s) _assert_log(t, s, __FILE__, __LINE__)

void _assert_logs_empty(bool fatal, bool before, const char * const file, const int line);
#define assert_logs_empty() _assert_logs_empty(true, false, __FILE__, __LINE__)
#define assert_logs_empty_before() _assert_logs_empty(true, true, __FILE__, __LINE__)

void logs_clear(void);

#endif // ASSERT_LOG_H

