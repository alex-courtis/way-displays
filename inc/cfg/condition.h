#ifndef CFG_CONDITION_H
#define CFG_CONDITION_H

#include <stdbool.h>

#include "enum.h"

struct CfgCondition {
	const struct Sset *plugged;
	const struct Sset *unplugged;
	enum ConditionLid lid;
};

struct CfgCondition *cfg_condition_init(void);

const struct Pset *cfg_condition_pset_init(void);

struct CfgCondition *cfg_condition_clone(const struct CfgCondition* const from);

// evaluates a condition.
// NULL condition is always evaluated to be false.
bool cfg_condition_true(const struct CfgCondition *condition);

void cfg_condition_free(struct CfgCondition *condition);

#endif // CFG_CONDITION_H
