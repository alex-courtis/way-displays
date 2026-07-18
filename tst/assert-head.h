#ifndef ASSERT_HEAD_H
#define ASSERT_HEAD_H

#include <cmocka.h>
#include <stdint.h>

#include "head.h"

void _assert_head_position(struct Head *head, int32_t x, int32_t y, const char * const file, const int line) {
	if (head->des.x != x || head->des.y != y) {
		cmocka_print_error("assert_head_position %s (%d, %d) != (%d, %d)\n", head->name, head->des.x, head->des.y, x, y);
		_fail(file, line);
	}
}

#define assert_head_position(h, x, y) _assert_head_position(h, x, y, __FILE__, __LINE__)

#endif // ASSERT_HEAD_H

