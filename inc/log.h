#ifndef LOG_H
#define LOG_H

enum log_level {
	LOG_LEVEL_DEBUG = 0,
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_ERROR
};

extern enum log_level log_threshold;

void log_info(const char *__restrict __format, ...);

void log_warn(const char *__restrict __format, ...);

void log_error(const char *__restrict __format, ...);

#endif // LOG_H

