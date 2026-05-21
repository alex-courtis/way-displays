#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include "slist.h"

enum LogThreshold {
	DEBUG = 1,
	INFO,
	WARNING,
	ERROR,
	FATAL,
	LOG_THRESHOLD_DEFAULT = INFO,
};

struct LogCapLine {
	char *line;
	enum LogThreshold threshold;
};


void log_set_threshold(enum LogThreshold threshold, bool cli);

enum LogThreshold log_get_threshold(void);

void log_set_times(bool times);


void log_(enum LogThreshold threshold, const char *__restrict __format, ...) __attribute__ ((__format__ (__printf__, 2, 3)));


void log_debug(const char *__restrict __format, ...)                         __attribute__ ((__format__ (__printf__, 1, 2)));

void log_info(const char *__restrict __format, ...)                          __attribute__ ((__format__ (__printf__, 1, 2)));

void log_warn(const char *__restrict __format, ...)                          __attribute__ ((__format__ (__printf__, 1, 2)));

void log_warn_errno(const char *__restrict __format, ...)                    __attribute__ ((__format__ (__printf__, 1, 2)));

void log_error(const char *__restrict __format, ...)                         __attribute__ ((__format__ (__printf__, 1, 2)));

void log_error_errno(const char *__restrict __format, ...)                   __attribute__ ((__format__ (__printf__, 1, 2)));

void log_fatal(const char *__restrict __format, ...)                         __attribute__ ((__format__ (__printf__, 1, 2)));

void log_fatal_errno(const char *__restrict __format, ...)                   __attribute__ ((__format__ (__printf__, 1, 2)));


void log_suppress_start(void);

void log_suppress_stop(void);


// caller must call stop and free lines
void log_cap_lines_start(struct SList **log_cap_lines);

void log_cap_lines_stop(struct SList **log_cap_lines);

void log_cap_lines_free(struct SList **log_cap_lines);

void log_cap_lines_playback(struct SList *log_cap_lines);


// vsprintf to a malloc'd buffer, does not mutate __args
char *vsprintf_alloc(const char *__restrict __format, va_list __args);

// vsnprintf to a malloc'd buffer, does not mutate __args
char *vsnprintf_alloc(size_t __maxlen, const char *__restrict __format, va_list __args);

// sprintf to a malloc'd buffer
char *sprintf_alloc(const char *__restrict __format, ...) __attribute__ ((__format__ (__printf__, 1, 2)));

// append to nullable s, returning a malloc'd buffer, freeing s
char *sprintf_append(char *__restrict s, const char *__restrict __format, ...) __attribute__ ((__format__ (__printf__, 2, 3)));


#endif // LOG_H

