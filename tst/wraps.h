#include <cmocka.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <unistd.h>

#define LOG_PRINT false

void __wrap_log_debug(const char *__restrict __format, const void *arg1, const void *arg2, const void *arg3, const void *arg4, ...) {
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

// mucking about with macros here is not worth the trouble
void expect_log_debug(const char *__restrict __format, const void *arg1, const void *arg2, const void *arg3, const void *arg4, ...) {
	expect_string(__wrap_log_debug, __format, __format);
	if (arg1)
		expect_string(__wrap_log_debug, arg1, arg1);
	else
		expect_any(__wrap_log_debug, arg1);
	if (arg2)
		expect_string(__wrap_log_debug, arg2, arg2);
	else
		expect_any(__wrap_log_debug, arg2);
	if (arg3)
		expect_string(__wrap_log_debug, arg3, arg3);
	else
		expect_any(__wrap_log_debug, arg3);
	if (arg4)
		expect_string(__wrap_log_debug, arg4, arg4);
	else
		expect_any(__wrap_log_debug, arg4);
}

void expect_log_info(const char *__restrict __format, const void *arg1, const void *arg2, const void *arg3, const void *arg4, ...) {
	expect_string(__wrap_log_info, __format, __format);
	if (arg1)
		expect_string(__wrap_log_info, arg1, arg1);
	else
		expect_any(__wrap_log_info, arg1);
	if (arg2)
		expect_string(__wrap_log_info, arg2, arg2);
	else
		expect_any(__wrap_log_info, arg2);
	if (arg3)
		expect_string(__wrap_log_info, arg3, arg3);
	else
		expect_any(__wrap_log_info, arg3);
	if (arg4)
		expect_string(__wrap_log_info, arg4, arg4);
	else
		expect_any(__wrap_log_info, arg4);
}

void expect_log_warn(const char *__restrict __format, const void *arg1, const void *arg2, const void *arg3, const void *arg4, ...) {
	expect_string(__wrap_log_warn, __format, __format);
	if (arg1)
		expect_string(__wrap_log_warn, arg1, arg1);
	else
		expect_any(__wrap_log_warn, arg1);
	if (arg2)
		expect_string(__wrap_log_warn, arg2, arg2);
	else
		expect_any(__wrap_log_warn, arg2);
	if (arg3)
		expect_string(__wrap_log_warn, arg3, arg3);
	else
		expect_any(__wrap_log_warn, arg3);
	if (arg4)
		expect_string(__wrap_log_warn, arg4, arg4);
	else
		expect_any(__wrap_log_warn, arg4);
}

void expect_log_error(const char *__restrict __format, const void *arg1, const void *arg2, const void *arg3, const void *arg4, ...) {
	expect_string(__wrap_log_error, __format, __format);
	if (arg1)
		expect_string(__wrap_log_error, arg1, arg1);
	else
		expect_any(__wrap_log_error, arg1);
	if (arg2)
		expect_string(__wrap_log_error, arg2, arg2);
	else
		expect_any(__wrap_log_error, arg2);
	if (arg3)
		expect_string(__wrap_log_error, arg3, arg3);
	else
		expect_any(__wrap_log_error, arg3);
	if (arg4)
		expect_string(__wrap_log_error, arg4, arg4);
	else
		expect_any(__wrap_log_error, arg4);
}

