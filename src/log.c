#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include "log.h"

#include "calc.h"
#include "types.h"

// we are single threaded
enum log_level log_threshold = LOG_LEVEL_INFO;
struct timeval tv;

void log_print(const char *prefix, const char *suffix, FILE *__restrict __stream, const char *__restrict __format, __gnuc_va_list __args) {

	gettimeofday(&tv, NULL);
	struct tm *tm = localtime(&tv.tv_sec);

	const char *format_stripped = &__format[0];
	while (format_stripped && format_stripped[0] == '\n') {
		fprintf(__stream, "%s [%02d:%02d:%02d.%03ld]\n", prefix, tm->tm_hour, tm->tm_min, tm->tm_sec, tv.tv_usec / 1000);
		format_stripped = &format_stripped[1];
	}

	fprintf(__stream, "%s [%02d:%02d:%02d.%03ld] %s", prefix, tm->tm_hour, tm->tm_min, tm->tm_sec, tv.tv_usec / 1000, suffix);
	vfprintf(__stream, format_stripped, __args);
	fprintf(__stream, "\n");
}

void log_info(const char *__restrict __format, ...) {
	if (log_threshold <= LOG_LEVEL_INFO) {
		va_list args;
		va_start(args, __format);
		log_print("I", "", stdout, __format, args);
		va_end(args);
	}
}

void log_warn(const char *__restrict __format, ...) {
	if (log_threshold <= LOG_LEVEL_WARNING) {
		va_list args;
		va_start(args, __format);
		log_print("W", "WARNING: ", stderr, __format, args);
		va_end(args);
	}
}

void log_error(const char *__restrict __format, ...) {
	if (log_threshold <= LOG_LEVEL_ERROR) {
		va_list args;
		va_start(args, __format);
		log_print("E", "ERROR: ", stderr, __format, args);
		va_end(args);
	}
}

