#ifndef ASSERTS_H
#define ASSERTS_H

#include <cmocka.h>
#include <stdio.h>
#include <string.h>
#include <wayland-util.h>

#include "cfg.h"
#include "head.h"
#include "marshalling.h"
#include "slist.h"
#include "stable.h"
#include "util.h"

void _assert_nul(const void *a, const char * const ae, const char * const file, const int line) {
	if (a) {
		cm_print_error("%s is not NULL\n", ae);
		_fail(file, line);
	}
}
#define assert_nul(a) _assert_nul(a, #a, __FILE__, __LINE__)

void _assert_non_nul(const void *a, const char * const ae, const char * const file, const int line) {
	if (!a) {
		cm_print_error("%s is NULL\n", ae);
		_fail(file, line);
	}
}
#define assert_non_nul(a) _assert_non_nul(a, #a, __FILE__, __LINE__)

void _assert_str_equal(const char * const a, const char * const ae, const char * const b, const char * const be, const char * const file, const int line) {
	if (!a && !b)
		return;
	_assert_non_nul(a, ae, file, line);
	_assert_non_nul(b, be, file, line);
	_assert_string_equal(a, b, file, line);
}
#define assert_str_equal(a, b) _assert_str_equal(a, #a, b, #b, __FILE__, __LINE__)

void _assert_str_equal_n(const char * const a, const char * const ae, const char * const b, const char * const be, const size_t n, const char * const file, const int line) {
	if (!a && !b)
		return;
	_assert_non_nul(a, ae, file, line);
	_assert_non_nul(b, be, file, line);
	if (strncmp(a, b, n) != 0) {
		cm_print_error("\"%.*s\" != \"%.*s\"\n", (int)n, a, (int)n, b);
		_fail(file, line);
	}
}

#define assert_str_equal_n(a, b, n) _assert_str_equal_n(a, #a, b, #b, n, __FILE__, __LINE__)

void _assert_wl_fixed_t_equal_double(wl_fixed_t a, double b, const char * const file, const int line) {

	if (a != wl_fixed_from_double(b)) {
		cm_print_error("%g != %g\n", wl_fixed_to_double(a), b);
		_fail(file, line);
	}
}
#define assert_wl_fixed_t_equal_double(a, b) _assert_wl_fixed_t_equal_double(a, b, __FILE__, __LINE__)

void _assert_heads_order(struct SList *a, struct SList *b, const char * const file, const int line) {
	if (!slist_equal(a, b, NULL)) {
		char actual[2048];
		char *ap = actual;
		*ap = '\0';
		for (struct SList *i = a; i; i = i->nex) {
			struct Head *head = (struct Head*)i->val;
			ap += sprintf(ap, "\n .name = '%s', .description = '%s',", head->name, head->description);
		}

		char expected[2048];
		char *ep = expected;
		*ep = '\0';
		for (struct SList *i = b; i; i = i->nex) {
			struct Head *head = (struct Head*)i->val;
			ep += sprintf(ep, "\n .name = '%s', .description = '%s',", head->name, head->description);
		}

		cm_print_error("assert_heads_order\nactual:%s\nexpected:%s\n\n", actual, expected);
		_fail(file, line);
	}
}
#define assert_heads_order(a, b) _assert_heads_order(a, b, __FILE__, __LINE__)

void _assert_head_position(struct Head *head, int32_t x, int32_t y, const char * const file, const int line) {
	if (head->desired.x != x || head->desired.y != y) {
		cm_print_error("assert_head_position %s (%d, %d) != (%d, %d)\n", head->name, head->desired.x, head->desired.y, x, y);
		_fail(file, line);
	}
}

#define assert_head_position(h, x, y) _assert_head_position(h, x, y, __FILE__, __LINE__)

void _assert_equal_cfg(struct Cfg *a, struct Cfg *b, const char * const file, const int line) {
	if (!cfg_equal(a, b)) {
		cm_print_error("assert_cfg_equal\ncfg.actual:\n%s\ncfg.expected:\n%s\n", marshal_cfg(a), marshal_cfg(b));
		write_file("cfg.actual", marshal_cfg(a));
		write_file("cfg.expected", marshal_cfg(b));
		_fail(file, line);
	}
}

#define assert_cfg_equal(a, b) _assert_equal_cfg(a, b, __FILE__, __LINE__)

void _assert_log(enum LogThreshold t, const char* s, const char * const file, const int line);
#define assert_log(t, s) _assert_log(t, s, __FILE__, __LINE__)

void _assert_logs_empty(const char * const file, const int line);
#define assert_logs_empty() _assert_logs_empty(__FILE__, __LINE__)

int expect_stable_equal(const LargestIntegralType value,
		const LargestIntegralType check_value_data) {
	return stable_equal((struct STable*)value, (struct STable*)check_value_data, NULL);
}

int expect_stable_equal_strcmp(const LargestIntegralType value,
		const LargestIntegralType check_value_data) {
	return stable_equal((struct STable*)value, (struct STable*)check_value_data, fn_comp_equals_strcmp);
}

#endif // ASSERTS_H

