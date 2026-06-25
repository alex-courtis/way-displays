#ifndef CONDITIONS_H
#define CONDITIONS_H

#include <stdbool.h>

#include "pset.h"

enum ConditionLid {
	LID_CLOSED = 1,
	LID_OPEN,
	LID_NOT_PRESENT,
};

struct Condition {
	struct SList *plugged;
	struct SList *unplugged;
	enum ConditionLid lid;
};

const struct PSet *condition_pset_init(void);

// evaluates a condition.
// NULL condition is always evaluated to be true.
bool condition_evaluate(const struct Condition *condition);

// evaluates a list of conditions.
// the result is equal to OR of all conditions in list.
// empty set (NULL) is always evaluated to be true.
bool condition_list_evaluate(const struct PSet* const conditions);

void condition_free(struct Condition *condition);

#endif // CONDITIONS_H
