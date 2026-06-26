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
	const struct SSet *plugged;
	const struct SSet *unplugged;
	enum ConditionLid lid;
};

struct Condition *condition_init(void);

const struct PSet *condition_pset_init(void);

struct Condition *condition_clone(const struct Condition* const from);

// evaluates a condition.
// NULL condition is always evaluated to be true.
bool condition_evaluate(const struct Condition *condition);

// evaluates a list of conditions.
// the result is equal to OR of all conditions in list.
// empty set (NULL) is always evaluated to be true.
bool condition_set_evaluate(const struct PSet* const conditions);

void condition_free(struct Condition *condition);

#endif // CONDITIONS_H
