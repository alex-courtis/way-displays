#ifndef CONDITIONS_H
#define CONDITIONS_H

#include <stdbool.h>
#include "slist.h"

enum ConditionLid {
	CLOSED = 1,
	OPEN,
};

struct Condition {
	struct SList *plugged;
	struct SList *unplugged;
	enum ConditionLid lid;
};

void condition_free(const void *data);

void* condition_clone(const void *data);

bool condition_equal(const void *a, const void *b);

// evaluates a condition.
// NULL condition is always evaluated to be true.
bool condition_evaluate(const struct Condition *condition);

// evaluates a list of conditions.
// the result is equal to OR of all conditions in list.
// empty list (NULL) is always evaluated to be true.
bool condition_list_evaluate(const struct SList *conditions);

#endif // CONDITIONS_H
