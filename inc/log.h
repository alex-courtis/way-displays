#ifndef LOG_H
#define LOG_H

#include <stdbool.h>

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
extern struct SList *log_cap_lines;

void log_set_threshold(enum LogThreshold threshold, bool cli);

void log_set_times(bool times);

void log_(enum LogThreshold threshold, const char *__restrict __format, ...);

void log_debug(const char *__restrict __format, ...);

void log_debug_nocap(const char *__restrict __format, ...);

void log_info(const char *__restrict __format, ...);

void log_warn(const char *__restrict __format, ...);

void log_warn_errno(const char *__restrict __format, ...);

void log_error(const char *__restrict __format, ...);

void log_error_errno(const char *__restrict __format, ...);

void log_suppress_start(void);

void log_suppress_stop(void);

void log_capture_start(void);

void log_capture_stop(void);

void log_capture_clear(void);

void log_capture_playback(void);

#endif // LOG_H

