#ifndef ASSERTS_LOG_H
#define ASSERTS_LOG_H

#include "log.h"

void _assert_log(enum LogThreshold t, const char* s, const char * const file, const int line);
#define assert_log(t, s) _assert_log(t, s, __FILE__, __LINE__)

void _assert_logs_empty(const char * const file, const int line);
#define assert_logs_empty() _assert_logs_empty(__FILE__, __LINE__)

#endif // ASSERTS_LOG_H

