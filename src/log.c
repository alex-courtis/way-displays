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
	bool suppressing;
};
struct LogActive active = {
	.threshold = LOG_THRESHOLD_DEFAULT,
	.threshold_cli = false,
	.times = false,
	.suppressing = false,
};

struct SList *log_cap_lines_active = NULL;

char threshold_char[] = {
	'?',
	'D',
	'I',
	'W',
	'E',
	'F',
};

char *threshold_prefix[] = {
	"",
	"",
	"",
	"WARNING: ",
	"ERROR: ",
	"FATAL: ",
};

void print_time(enum LogThreshold threshold, FILE *__restrict __stream) {
	static char buf[16];
	static time_t t;

	t = time(NULL);

	strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&t));

	fprintf(__stream, "%c [%s] ", threshold_char[threshold], buf);
}

void capture_line(struct SList **lines, enum LogThreshold threshold, char *l) {
	struct LogCapLine *line = calloc(1, sizeof(struct LogCapLine));
	line->line = strdup(l);
	line->threshold = threshold;
	slist_append(lines, line);
}

void print_raw(enum LogThreshold threshold, bool prefix, const char *l) {
	static FILE *stream;

	stream = threshold >= ERROR ? stderr : stdout;

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

	for (struct SList *i = log_cap_lines_active; i; i = i->nex) {
		capture_line(i->val, threshold, l);
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

void log_fatal(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	print_log(FATAL, 0, __format, args);
	va_end(args);
}

void log_fatal_errno(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	print_log(FATAL, errno, __format, args);
	va_end(args);
}

void log_suppress_start(void) {
	active.suppressing = true;
}

void log_suppress_stop(void) {
	active.suppressing = false;
}

void log_cap_line_free(const void *data) {
	const struct LogCapLine *line = data;

	if (!line) {
		return;
	}

	if (line->line) {
		free(line->line);
	}

	free((struct LogCapLine*)line);
}

void log_cap_lines_playback(struct SList *lines) {
	if (!lines)
		return;

	for (struct SList *i = lines; i; i = i->nex) {
		struct LogCapLine *line = i->val;
		if (!line)
			continue;

		print_raw(line->threshold, true, line->line);
	}
}

void log_cap_lines_start(struct SList **lines) {
	slist_append(&log_cap_lines_active, lines);
}

void log_cap_lines_stop(struct SList **lines) {
	slist_remove_all(&log_cap_lines_active, NULL, lines);
}

void log_cap_lines_free(struct SList **lines) {
	slist_free_vals(lines, log_cap_line_free);
}
