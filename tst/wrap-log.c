#include "tst.h" // IWYU pragma: keep

#include <cmocka.h>
#include <stdio.h>
#include <stdbool.h>

#include "log.h"

void __wrap_log_set_threshold(enum LogThreshold threshold, bool cli) {
	check_expected(threshold);
	check_expected(cli);
}

void __wrap_log_(enum LogThreshold threshold, const char *__restrict __format, const void *arg1, const void *arg2, const void *arg3, const void *arg4, ...) {
	if (LOG_PRINT) {
		fprintf(stderr, __format, arg1, arg2, arg3, arg4);
		fprintf(stderr, "\n");
	}
	check_expected(threshold);
	check_expected(__format);
	check_expected(arg1);
	check_expected(arg2);
	check_expected(arg3);
	check_expected(arg4);
}

void __wrap_log_info(const char *__restrict __format, const void *arg1, const void *arg2, const void *arg3, const void *arg4, ...) {
	if (LOG_PRINT) {
		fprintf(stderr, __format, arg1, arg2, arg3, arg4);
		fprintf(stderr, "\n");
	}
	check_expected(__format);
	check_expected(arg1);
	check_expected(arg2);
	check_expected(arg3);
	check_expected(arg4);
}

void __wrap_log_warn(const char *__restrict __format, const void *arg1, const void *arg2, const void *arg3, const void *arg4, ...) {
	if (LOG_PRINT) {
		fprintf(stderr, __format, arg1, arg2, arg3, arg4);
		fprintf(stderr, "\n");
	}
	check_expected(__format);
	check_expected(arg1);
	check_expected(arg2);
	check_expected(arg3);
	check_expected(arg4);
}

void __wrap_log_error(const char *__restrict __format, const void *arg1, const void *arg2, const void *arg3, const void *arg4, ...) {
	if (LOG_PRINT) {
		fprintf(stderr, __format, arg1, arg2, arg3, arg4);
		fprintf(stderr, "\n");
	}
	check_expected(__format);
	check_expected(arg1);
	check_expected(arg2);
	check_expected(arg3);
	check_expected(arg4);
}

void __wrap_log_error_nocap(const char *__restrict __format, const void *arg1, const void *arg2, const void *arg3, const void *arg4, ...) {
	if (LOG_PRINT) {
		fprintf(stderr, __format, arg1, arg2, arg3, arg4);
		fprintf(stderr, "\n");
	}
	check_expected(__format);
	check_expected(arg1);
	check_expected(arg2);
	check_expected(arg3);
	check_expected(arg4);
}

