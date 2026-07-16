#ifndef ASSERT_HEAD_H
#define ASSERT_HEAD_H

#include <cmocka.h>
#include <stdint.h>
#include <stdio.h>

#include "head.h"
#include "pset.h"

void _assert_head_position(struct Head *head, int32_t x, int32_t y, const char * const file, const int line) {
	if (head->desired.x != x || head->desired.y != y) {
		cmocka_print_error("assert_head_position %s (%d, %d) != (%d, %d)\n", head->name, head->desired.x, head->desired.y, x, y);
		_fail(file, line);
	}
}

#define assert_head_position(h, x, y) _assert_head_position(h, x, y, __FILE__, __LINE__)

#endif // ASSERT_HEAD_H

