#ifndef ASSERT_HEAD_H
#define ASSERT_HEAD_H

#include <cmocka.h>
#include <stdint.h>
#include <stdio.h>

#include "head.h"
#include "slist.h"

void _assert_heads_order(struct SList *a, struct SList *b, const char * const file, const int line) {
	if (!slist_equal(a, b, NULL)) {
		char actual[2048];
		char *ap = actual;
		*ap = '\0';
		for (struct SList *i = a; i; i = i->nex) {
			const struct Head *head = (struct Head*)i->val;
			ap += sprintf(ap, "\n .name = '%s', .description = '%s',", head->name, head->description);
		}

		char expected[2048];
		char *ep = expected;
		*ep = '\0';
		for (struct SList *i = b; i; i = i->nex) {
			const struct Head *head = (struct Head*)i->val;
			ep += sprintf(ep, "\n .name = '%s', .description = '%s',", head->name, head->description);
		}

		cmocka_print_error("assert_heads_order\nACTUAL:%s\nEXPECTED:%s\n\n", actual, expected);
		_fail(file, line);
	}
}
#define assert_heads_order(a, b) _assert_heads_order(a, b, __FILE__, __LINE__)

void _assert_head_position(struct Head *head, int32_t x, int32_t y, const char * const file, const int line) {
	if (head->desired.x != x || head->desired.y != y) {
		cmocka_print_error("assert_head_position %s (%d, %d) != (%d, %d)\n", head->name, head->desired.x, head->desired.y, x, y);
		_fail(file, line);
	}
}

#define assert_head_position(h, x, y) _assert_head_position(h, x, y, __FILE__, __LINE__)

#endif // ASSERT_HEAD_H

