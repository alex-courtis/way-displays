#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <time.h>
#include <unistd.h>

#include "log.h"

#include "slist.h"

#define MAX_LINE_LEN 65536

struct LogActive {
	enum LogThreshold threshold;
	bool threshold_cli;
	bool prefix;
	bool suppressing;
};
static struct LogActive active = {
	.threshold = LOG_THRESHOLD_DEFAULT,
	.threshold_cli = false,
	.prefix = false,
	.suppressing = false,
};

static struct SList *log_cap_lines_active = NULL;

// all thresholds, start of line
static const char threshold_char[] = {
	'?',
	'D',
	'I',
	'W',
	'E',
	'F',
};

// not for all thresholds
static const char *threshold_label[] = {
	NULL,
	NULL,
	NULL,
	"WARNING: ",
	"ERROR: ",
	"FATAL: ",
};

// all but info
static const char *threshold_colours[] = {
	NULL,
	"\x1B[1;36m", // cyan    debug
	NULL,         //         info
	"\x1B[1;35m", // magenta warning
	"\x1B[1;31m", // red     error
	"\x1B[1;33m", // yellow  fatal
};
static const char reset_colour[] = "\x1B[0m";

static void capture_line(enum LogThreshold threshold, const char *l) {
	for (struct SList *i = log_cap_lines_active; i; i = i->nex) {
		struct LogCapLine *line = calloc(1, sizeof(struct LogCapLine));
		line->line = strdup(l);
		line->threshold = threshold;

		slist_append(i->val, line);
	}
}

static void print_line(const enum LogThreshold threshold, const char *l) {

	if (threshold < active.threshold || active.suppressing)
		return;

	FILE *stream = threshold >= ERROR ? stderr : stdout;
	const int fd = threshold >= ERROR ? STDERR_FILENO : STDOUT_FILENO;

	// colour only for terminal output
	const char *colour = NULL;
	if (isatty(fd))
		colour = threshold_colours[threshold];

	// maybe colour for entire line
	if (colour)
		fprintf(stream, "%s", colour);

	// maybe one char threshold and time
	if (active.prefix) {

		static char buf_time[16];

		time_t t = time(NULL);

		strftime(buf_time, sizeof(buf_time), "%H:%M:%S", localtime(&t));

		fprintf(stream, "%c [%s] ", threshold_char[threshold], buf_time);
	}

	if (l) {

		// maybe label on non-empty lines
		if (l[0] != '\0' && threshold_label[threshold]) {
			fprintf(stream, "%s", threshold_label[threshold]);
		}

		// line contents
		fprintf(stream, "%s%s\n", l, colour ? reset_colour : "");

	} else {

		// empty line
		fprintf(stream, "%s\n", colour ? reset_colour : "");
	}
}

// allocate a buffer containing the line's content, maybe printing and capturing
// allocating the buffer is slower than direct vfprintf, however it is the safer way to capture
static void log_line(const enum LogThreshold threshold, const int eno, const char *__restrict __format, va_list __args) {
	char *l;

	// allocate the line for output and capture
	if (__format && __args)
		l = vsnprintf_alloc(MAX_LINE_LEN, __format, __args);
	else
		l = strdup("");

	// append errno
	if (eno)
		l = sprintf_append(l, ": %d %s", eno, strerror(eno));


	// output depending on threshold
	print_line(threshold, l);

	// capture content regardless of threshold
	if (log_cap_lines_active)
		capture_line(threshold, l);

	free(l);
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

enum LogThreshold log_get_threshold(void) {
	return active.threshold;
}

void log_set_prefix(bool prefix) {
	active.prefix = prefix;
}

void log_(enum LogThreshold threshold, const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	log_line(threshold, 0, __format, args);
	va_end(args);
}

void log_debug(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	log_line(DEBUG, 0, __format, args);
	va_end(args);
}

void log_info(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	log_line(INFO, 0, __format, args);
	va_end(args);
}

void log_warn(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	log_line(WARNING, 0, __format, args);
	va_end(args);
}

void log_warn_errno(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	log_line(WARNING, errno, __format, args);
	va_end(args);
}

void log_error(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	log_line(ERROR, 0, __format, args);
	va_end(args);
}

void log_error_errno(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	log_line(ERROR, errno, __format, args);
	va_end(args);
}

void log_fatal(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	log_line(FATAL, 0, __format, args);
	va_end(args);
}

void log_fatal_errno(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	log_line(FATAL, errno, __format, args);
	va_end(args);
}

void log_suppress_start(void) {
	active.suppressing = true;
}

void log_suppress_stop(void) {
	active.suppressing = false;
}

static void log_cap_line_free(const void *data) {
	const struct LogCapLine *line = data;

	if (!line) {
		return;
	}

	if (line->line) {
		free(line->line);
	}

	free((struct LogCapLine*)line);
}

void log_cap_lines_playback(const struct SList *lines) {
	if (!lines)
		return;

	for (const struct SList *i = lines; i; i = i->nex) {
		const struct LogCapLine *line = i->val;
		if (!line)
			continue;

		print_line(line->threshold, line->line);
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

char *vsprintf_alloc(const char *__restrict __format, va_list __args) {

	va_list args;
	va_copy(args, __args);
	size_t len = vsnprintf(NULL, 0, __format, args);
	va_end(args);

	char *str = calloc(len + 1, sizeof(char));

	va_copy(args, __args);
	vsnprintf(str, len + 1, __format, args);
	va_end(args);

	return str;
}

char *vsnprintf_alloc(size_t __maxlen, const char *__restrict __format, va_list __args) {

	va_list args;
	va_copy(args, __args);
	size_t raw = vsnprintf(NULL, 0, __format, args);
	va_end(args);

	size_t len = MIN(raw, __maxlen);

	char *str = calloc(len + 1, sizeof(char));

	va_copy(args, __args);
	vsnprintf(str, len + 1, __format, args);
	va_end(args);

	return str;
}

char *sprintf_alloc(const char *__restrict __format, ...) {

	va_list args;
	va_start(args, __format);
	size_t len = vsnprintf(NULL, 0, __format, args);
	va_end(args);

	char *str = calloc(len + 1, sizeof(char));

	va_start(args, __format);
	vsnprintf(str, len + 1, __format, args);
	va_end(args);

	return str;
}

char *sprintf_append(char *__restrict s, const char *__restrict __format, ...) {
	size_t l_left = s ? strlen(s) : 0;

	va_list args;
	va_start(args, __format);
	size_t l_right = vsnprintf(NULL, 0, __format, args);
	va_end(args);

	char *left = calloc(l_left + l_right + 1, sizeof(char));

	char *right = l_left ? stpncpy(left, s, l_left + 1) : left;

	va_start(args, __format);
	vsnprintf(right, l_right + 1, __format, args);
	va_end(args);

	if (s)
		free(s);

	return left;
}
