#ifndef EXPECTS_H
#define EXPECTS_H

#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

#include <cmocka.h>

#include "log.h"

#define expect_log(fn, f, a1, a2, a3, a4) \
	if (f) \
		expect_string(fn, __format, f); \
	else \
		expect_any(fn, __format); \
	if (a1) \
		expect_string(fn, arg1, a1); \
	else \
		expect_any(fn, arg1); \
	if (a2) \
		expect_string(fn, arg2, a2); \
	else \
		expect_any(fn, arg2); \
	if (a3) \
		expect_string(fn, arg3, a3); \
	else \
		expect_any(fn, arg3); \
	if (a4) \
		expect_string(fn, arg4, a4); \
	else \
		expect_any(fn, arg4);

#define expect_log_debug(f, a1, a2, a3, a4) \
	expect_log(__wrap_log_debug, f, a1, a2, a3, a4)

#define expect_log_info(f, a1, a2, a3, a4) \
	expect_log(__wrap_log_info, f, a1, a2, a3, a4)

#define expect_log_warn(f, a1, a2, a3, a4) \
	expect_log(__wrap_log_warn, f, a1, a2, a3, a4)

#define expect_log_error(f, a1, a2, a3, a4) \
	expect_log(__wrap_log_error, f, a1, a2, a3, a4)

#define expect_log_error_nocap(f, a1, a2, a3, a4) \
	expect_log(__wrap_log_error_nocap, f, a1, a2, a3, a4)

#define expect_log_(t, f, a1, a2, a3, a4) \
	expect_value(__wrap_log_, threshold, t); \
	expect_log(__wrap_log_, f, a1, a2, a3, a4)

#endif // EXPECTS_H
