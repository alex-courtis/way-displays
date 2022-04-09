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

void log_efl_(enum LogThreshold threshold, int eno, char *file, int line, const char *__restrict __format, ...);

#define log_debug(__format, ...) log_efl_(DEBUG, 0, (char*)__FILE__, __LINE__, __format, ##__VA_ARGS__)

#define log_info(__format, ...) log_efl_(INFO, 0, NULL, 0, __format, ##__VA_ARGS__)

#define log_warn(__format, ...) log_efl_(WARNING, 0, NULL, 0, __format, ##__VA_ARGS__)

#define log_warn_errno(__format, ...) log_efl_(WARNING, errno, NULL, 0, __format, ##__VA_ARGS__)

#define log_error(__format, ...) log_efl_(ERROR, 0, (char*)__FILE__, __LINE__, __format, ##__VA_ARGS__)

#define log_error_errno(__format, ...) log_efl_(ERROR, errno, (char*)__FILE__, __LINE__, __format, ##__VA_ARGS__)

void log_suppress_start(void);

void log_suppress_end(void);

void log_capture_start(void);

void log_capture_end(void);

void log_capture_reset(void);

void log_capture_playback(void);

#endif // LOG_H

