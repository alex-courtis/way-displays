#include "tst.h" // IWYU pragma: keep

#include <cmocka.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "convert.h"
#include "log.h"
#include "util.h"

// 0 unused, 1 DEBUG, 5 ERROR
static char b[6][262144] = { 0 };
static char *bp[6] = { 0 };

void _assert_log(enum LogThreshold t, const char *s, const char * const file, const int line) {
	if (bp[t]) {
		bp[t] = NULL;
		if (strcmp(b[t], s) != 0) {
			cm_print_error("assert_log\nlog.actual:\n\"%s\"\nlog.expected:\n\"%s\"\n", b[t], s);
			write_file("log.actual", b[t]);
			write_file("log.expected", s);
			_fail(file, line);
		}
	} else {
		_assert_string_equal("", s, file, line);
	}
}

void _assert_logs_empty(const char * const file, const int line) {
	bool empty = true;
	for (enum LogThreshold t = DEBUG; t <= ERROR; t++) {
		if (bp[t]) {
			bp[t] = NULL;
			cm_print_error("\nunexpected log %s:\n\"%s\"\n", log_threshold_name(t), b[t]);
			empty = false;
		}
	}
	if (!empty) {
		_fail(file, line);
	}
}

void _log(enum LogThreshold t, const char *__restrict __format, va_list __args) {
	static char *printed;

	if (!bp[t]) {
		bp[t] = b[t];
	}

	printed = bp[t];

	bp[t] += vsnprintf(bp[t], sizeof(b[t]) - (bp[t] - b[t]), __format, __args);

	if (LOG_PRINT) {
		fprintf(stderr, "%s\n", printed);
	}

	bp[t] += snprintf(bp[t], sizeof(b[t]) - (bp[t] - b[t]), "\n");
}


void __wrap_log_set_threshold(enum LogThreshold threshold, bool cli) {
	check_expected(threshold);
	check_expected(cli);
}

void __wrap_log_(enum LogThreshold t, const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	_log(t, __format, args);
	va_end(args);
}

void __wrap_log_debug(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	_log(DEBUG, __format, args);
	va_end(args);
}

void __wrap_log_info(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	_log(INFO, __format, args);
	va_end(args);
}

void __wrap_log_warn(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	_log(WARNING, __format, args);
	va_end(args);
}

void __wrap_log_error(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	_log(ERROR, __format, args);
	va_end(args);
}

void __wrap_log_error_errno(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	_log(ERROR, __format, args);
	va_end(args);
}

void __wrap_log_fatal(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	_log(FATAL, __format, args);
	va_end(args);
}

void __wrap_log_fatal_errno(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	_log(FATAL, __format, args);
	va_end(args);
}

