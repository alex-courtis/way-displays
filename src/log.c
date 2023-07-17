#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log.h"

#include "slist.h"

#define LS 16384

struct LogActive {
	enum LogThreshold threshold;
	bool threshold_cli;
	bool times;
	bool capturing;
	bool suppressing;
};
struct LogActive active = {
	.threshold = LOG_THRESHOLD_DEFAULT,
	.threshold_cli = false,
	.times = false,
	.capturing = false,
	.suppressing = false,
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

void print_raw(enum LogThreshold threshold, bool prefix, const char *l) {
	static FILE *stream;

	stream = threshold == ERROR ? stderr : stdout;

	if (threshold >= active.threshold && !active.suppressing) {
		if (active.times) {
			print_time(threshold, stream);
		}
		if (l[0] != '\0') {
			fprintf(stream, "%s%s\n", prefix ? threshold_prefix[threshold] : "", l);
		} else {
			fprintf(stream, "\n");
		}
	}
}

void print_line(enum LogThreshold threshold, bool prefix, int eno, const char *__restrict __format, va_list __args) {
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

	print_raw(threshold, prefix, l);
}

void print_log(enum LogThreshold threshold, int eno, const char *__restrict __format, va_list __args) {
	static const char *format;

	format = __format;
	while (*format == '\n') {
		print_line(threshold, false, 0, NULL, __args);
		format++;
	}
	print_line(threshold, true, eno, format, __args);
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

void log_set_times(bool times) {
	active.times = times;
}

void log_(enum LogThreshold threshold, const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	print_log(threshold, 0, __format, args);
	va_end(args);
}

void log_debug(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	print_log(DEBUG, 0, __format, args);
	va_end(args);
}

void log_info(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	print_log(INFO, 0, __format, args);
	va_end(args);
}

void log_warn(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	print_log(WARNING, 0, __format, args);
	va_end(args);
}

void log_warn_errno(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	print_log(WARNING, errno, __format, args);
	va_end(args);
}

void log_error(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	print_log(ERROR, 0, __format, args);
	va_end(args);
}

void log_error_errno(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	print_log(ERROR, errno, __format, args);
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

void log_suppress_start(void) {
	active.suppressing = true;
}

void log_suppress_stop(void) {
	active.suppressing = false;
}

void log_capture_start(void) {
	active.capturing = true;
}

void log_capture_stop(void) {
	active.capturing = false;
}

void log_capture_clear(void) {
	slist_free_vals(&log_cap_lines, free_log_cap_line);
}

void log_capture_playback(void) {
	bool was_capturing = active.capturing;
	active.capturing = false;

	for (struct SList *i = log_cap_lines; i; i = i->nex) {
		struct LogCapLine *cap_line = i->val;
		if (!cap_line)
			continue;

		print_raw(cap_line->threshold, true, cap_line->line);
	}

	active.capturing = was_capturing;
}

