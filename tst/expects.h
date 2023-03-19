#ifndef EXPECTS_H
#define EXPECTS_H

#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

#include <cmocka.h>

#include "log.h"

void expect_log_(enum LogThreshold threshold, const char *__restrict __format, const void *arg1, const void *arg2, const void *arg3, const void *arg4, ...);
void expect_log_debug(const char *__restrict __format, const void *arg1, const void *arg2, const void *arg3, const void *arg4, ...);
void expect_log_info(const char *__restrict __format, const void *arg1, const void *arg2, const void *arg3, const void *arg4, ...);
void expect_log_warn(const char *__restrict __format, const void *arg1, const void *arg2, const void *arg3, const void *arg4, ...);
void expect_log_error(const char *__restrict __format, const void *arg1, const void *arg2, const void *arg3, const void *arg4, ...);
void expect_log_error_nocap(const char *__restrict __format, const void *arg1, const void *arg2, const void *arg3, const void *arg4, ...);

#endif // EXPECTS_H
