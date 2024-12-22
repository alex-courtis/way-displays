#ifndef LOG_H
#define LOG_H

#include <stdbool.h>

#include "slist.h"

enum LogThreshold {
	DEBUG = 1,
	INFO,
	WARNING,
	ERROR,
	LOG_THRESHOLD_DEFAULT = INFO,
};

struct LogCapLine {
	char *line;
	enum LogThreshold threshold;
};


void log_set_threshold(enum LogThreshold threshold, bool cli);

void log_set_times(bool times);


void log_(enum LogThreshold threshold, const char *__restrict __format, ...);


void log_debug(const char *__restrict __format, ...);

void log_info(const char *__restrict __format, ...);

void log_warn(const char *__restrict __format, ...);

void log_warn_errno(const char *__restrict __format, ...);

void log_error(const char *__restrict __format, ...);

void log_error_errno(const char *__restrict __format, ...);


void log_suppress_start(void);

void log_suppress_stop(void);


// caller must call stop and free lines
void log_cap_lines_start(struct SList **log_cap_lines);

void log_cap_lines_stop(struct SList **log_cap_lines);

void log_cap_lines_free(struct SList **log_cap_lines);

void log_cap_lines_playback(struct SList *log_cap_lines);

#endif // LOG_H

