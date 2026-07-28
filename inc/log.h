#ifndef LOG_H
#define LOG_H

#include <stdbool.h>

#include "enum.h"
#include "plist.h"

struct LogCapLine {
	char *line;
	enum LogThreshold threshold;
};


// call before any logging ops
void log_init(void);

// clear any resources before shutdown
void log_destroy(void);

void log_set_threshold(enum LogThreshold threshold, bool cli);

enum LogThreshold log_get_threshold(void);

void log_set_prefix(bool prefix);


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


// create a line, contents strdup'd
struct LogCapLine *log_cap_line_init(const enum LogThreshold t, const char *line);

// create a list to contain LogCapLine, caller must plist_free_vals
const struct Plist *log_cap_line_plist_init(void);

// caller must call stop before freeing log_cap_lines
void log_cap_lines_start(const struct Plist *log_cap_lines);

// stops only, does not free
void log_cap_lines_stop(const struct Plist *log_cap_lines);

// any set of lines
void log_cap_lines_playback(const struct Plist *log_cap_lines);


#endif // LOG_H

