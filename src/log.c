// IWYU pragma: no_include <bits/types/struct_tm.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "log.h"

#include "list.h"

#define LS 16384

struct active {
	enum LogThreshold threshold;
	bool threshold_cli;
	bool times;
	bool capturing;
};
struct active active = {
	.threshold = LOG_THRESHOLD_DEFAULT,
	.threshold_cli = false,
	.times = false,
	.capturing = false,
};

struct SList *log_cap_lines = NULL;

char threshold_char[] = {
	'?',
	'D',
	'I',
	'W',
	'E',
};

char *threshold_prefix[] = {
	"",
	"",
	"",
	"WARNING: ",
	"ERROR: ",
};

void print_time(enum LogThreshold threshold, FILE *__restrict __stream) {
	static char buf[16];
	static time_t t;

	t = time(NULL);

	strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&t));

	fprintf(__stream, "%c [%s] ", threshold_char[threshold], buf);
}

void capture_line(enum LogThreshold threshold, char *l) {
	struct LogCapLine *cap_line = calloc(1, sizeof(struct LogCapLine));
	cap_line->line = strdup(l);
	cap_line->threshold = threshold;
	slist_append(&log_cap_lines, cap_line);
}

void print_line(enum LogThreshold threshold, const char *prefix, int eno, FILE *__restrict __stream, const char *__restrict __format, va_list __args) {
	static char l[LS];
	static size_t n;

	n = 0;
	l[0] = '\0';

	if (__format) {
		n += vsnprintf(l + n, LS - n, __format, __args);
	}
	if (eno) {
		n += snprintf(l + n, LS - n, ": %d %s", eno, strerror(eno));
	}

	if (n >= LS) {
		sprintf(l + LS - 4, "...");
	}

	if (active.capturing) {
		capture_line(threshold, l);
	}

	if (threshold >= active.threshold) {
		if (active.times) {
			print_time(threshold, __stream);
		}
		if (l[0] != '\0') {
			fprintf(__stream, "%s%s\n", prefix, l);
		} else {
			fprintf(__stream, "\n");
		}
	}
}

void print_log(enum LogThreshold threshold, int eno, FILE *__restrict __stream, const char *__restrict __format, va_list __args) {
	static const char *format;

	format = __format;
	while (*format == '\n') {
		print_line(threshold, "", 0, __stream, NULL, __args);
		format++;
	}
	print_line(threshold, threshold_prefix[threshold], eno, __stream, format, __args);
}

void log_set_threshold(enum LogThreshold threshold, bool cli) {
	if (!threshold) {
		return;
	}

	if (!active.threshold_cli || cli) {
		active.threshold = threshold;
		active.threshold_cli = cli;
	}
}

enum LogThreshold log_get_threshold() {
	return active.threshold;
}

void log_set_times(bool times) {
	active.times = times;
}

void log_(enum LogThreshold threshold, const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	print_log(threshold, 0, stdout, __format, args);
	va_end(args);
}

void log_debug(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	print_log(DEBUG, 0, stdout, __format, args);
	va_end(args);
}

void log_info(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	print_log(INFO, 0, stdout, __format, args);
	va_end(args);
}

void log_warn(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	print_log(WARNING, 0, stderr, __format, args);
	va_end(args);
}

void log_warn_errno(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	print_log(WARNING, errno, stderr, __format, args);
	va_end(args);
}

void log_error(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	print_log(ERROR, 0, stderr, __format, args);
	va_end(args);
}

void log_error_errno(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	print_log(ERROR, errno, stderr, __format, args);
	va_end(args);
}

void free_log_cap_line(void *data) {
	struct LogCapLine *cap_line = data;

	if (!cap_line) {
		return;
	}

	if (cap_line->line) {
		free(cap_line->line);
	}

	free(cap_line);
}

void log_capture_start() {
	active.capturing = true;
}

void log_capture_end() {
	active.capturing = false;
}

void log_capture_reset() {
	slist_free_vals(&log_cap_lines, free_log_cap_line);

	active.capturing = false;
}

