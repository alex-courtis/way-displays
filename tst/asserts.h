#ifndef ASSERTS_H
#define ASSERTS_H

#include <cmocka.h>
#include <stdio.h>
#include <wayland-util.h>

#include "cfg.h"
#include "head.h"
#include "slist.h"
#include "marshalling.h"

void cm_print_error(const char* const format, ...) CMOCKA_PRINTF_ATTRIBUTE(1, 2);

void _assert_wl_fixed_t_equal_double(wl_fixed_t a, double b, const char * const file, const int line) {

	if (a != wl_fixed_from_double(b)) {
		cm_print_error("%g != %g\n", wl_fixed_to_double(a), b);
		_fail(file, line);
	}
}
#define assert_wl_fixed_t_equal_double(a, b) _assert_wl_fixed_t_equal_double(a, b, __FILE__, __LINE__)

void _assert_heads_equal(struct SList *a, struct SList *b, const char * const file, const int line) {
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

		cm_print_error("assert_heads_equal\nactual:%s\nexpected:%s\n\n", actual, expected);
		_fail(file, line);
	}
}
#define assert_heads_equal(a, b) _assert_heads_equal(a, b, __FILE__, __LINE__)

void _assert_head_position(struct Head *head, int32_t x, int32_t y, const char * const file, const int line) {
	if (head->desired.x != x || head->desired.y != y) {
		cm_print_error("assert_head_position %s (%d, %d) != (%d, %d)\n", head->name, head->desired.x, head->desired.y, x, y);
		_fail(file, line);
	}
}

#define assert_head_position(h, x, y) _assert_head_position(h, x, y, __FILE__, __LINE__)

void _assert_equal_cfg(struct Cfg *a, struct Cfg *b, const char * const file, const int line) {
	if (!cfg_equal(a, b)) {
		cm_print_error("assert_cfg_equal\nactual:\n\n%s\nexpected:\n\n%s\n\n", marshal_cfg(a), marshal_cfg(b));
		_fail(file, line);
	}
}

#define assert_cfg_equal(a, b) _assert_equal_cfg(a, b, __FILE__, __LINE__)

void _assert_log(enum LogThreshold t, const char* s, const char * const file, const int line);
#define assert_log(t, s) _assert_log(t, s, __FILE__, __LINE__)

void _assert_logs_empty(const char * const file, const int line);
#define assert_logs_empty() _assert_logs_empty(__FILE__, __LINE__)

#endif // ASSERTS_H

