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

// TODO #259 this is still backwards
// fn_pred_p: true if all elements of the condition are met, NULL condition is always evaluated to be false.
bool cfg_condition_true(const struct CfgCondition *condition);

void cfg_condition_free(struct CfgCondition *condition);

#endif // CFG_CONDITION_H
