#include <assert.h>
#include <cmocka.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "convert.h"
#include "log.h"
#include "str.h"
#include "util-file.h"

#include "wrap-log.h"

// TODO use logs_clear at some point that identifies the bad test

// log space b is statically allocated and not cleared
// bp is used to indicate presence of logs
// logs are reset by clearing bp on assert_log and logs_clear

// 0 unused, 1 DEBUG, 5 FATAL
static char b[6][262144] = { 0 };
static char *bp[6] = { 0 };

void logs_clear(void) {
	for (enum LogThreshold t = DEBUG; t <= FATAL; t++) {
		bp[t] = NULL;
	}
}

void _assert_log(enum LogThreshold t, const char * s, const char * const file, const int line) {
	if (bp[t]) {
		bp[t] = NULL;
		if (strcmp(b[t], s) != 0) {
			cmocka_print_error("assert_log\nactual.log:\n\"%s\"\nexpected.log:\n\"%s\"\n", b[t], s);
			write_file("actual.log", b[t]);
			write_file("expected.log", s);
			_fail(file, line);
		}
	} else {
		_assert_string_equal("", s, file, line);
	}
}

void _assert_logs_empty(bool fatal, bool before, const char * const file, const int line) {
	bool empty = true;
	for (enum LogThreshold t = DEBUG; t <= FATAL; t++) {
		if (bp[t]) {
			bp[t] = NULL;
			char *file_name = sprintf_alloc("unexpected.%s.log", log_threshold_name(t));
			char *err = sprintf_alloc("%s%s:\n\"%s\"\n", before ? "Previous test left logs:\n" : "", file_name, b[t]);
			if (fatal) {
				fprintf(stderr, "%s:%d: %s", file, line, err);
			} else {
				cmocka_print_error("%s", err);
			}
			write_file(file_name, b[t]);
			free(file_name);
			empty = false;
		}
	}
	if (!empty) {
		if (fatal) {
			exit(1);
		} else {
			_fail(file, line);
		}
	}
}

static void _log(enum LogThreshold t, const char *__restrict __format, va_list __args) {
	const char *printed;

	if (!bp[t]) {
		bp[t] = b[t];
	}

	printed = bp[t];

	if (__format) {
		bp[t] += vsnprintf(bp[t], sizeof(b[t]) - (bp[t] - b[t]), __format, __args);
	} else {
		bp[t] += snprintf(bp[t], sizeof(b[t]) - (bp[t] - b[t]), "%s", "");
	}

	if (LOG_PRINT) {
		fprintf(stderr, "%s\n", printed);
	}

	bp[t] += snprintf(bp[t], sizeof(b[t]) - (bp[t] - b[t]), "\n");
}

void __wrap_log_set_threshold(enum LogThreshold threshold, bool cli) {
	check_expected_int(threshold);
	check_expected_int(cli);
}

enum LogThreshold __wrap_log_get_threshold(void) {
	return mock_type(enum LogThreshold);
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

