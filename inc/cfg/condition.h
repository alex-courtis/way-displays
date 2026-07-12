#ifndef CONDITIONS_H
#define CONDITIONS_H

#include <stdbool.h>

enum ConditionLid {
	LID_CLOSED = 1,
	LID_OPEN,
	LID_NOT_PRESENT,
};

struct Condition {
	const struct Sset *plugged;
	const struct Sset *unplugged;
	enum ConditionLid lid;
};

struct Condition *condition_init(void);

const struct Pset *condition_pset_init(void);

struct Condition *condition_clone(const struct Condition* const from);

// evaluates a condition.
// NULL condition is always evaluated to be false.
bool condition_true(const struct Condition *condition, const void* const unused);

void condition_free(struct Condition *condition);

#endif // CONDITIONS_H
