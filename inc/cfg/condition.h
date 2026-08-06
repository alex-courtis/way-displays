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

// fn_pred_p: true if any elements of the condition are not met, fail_lid_closed directs to unconditionally fail lid closed conditions
bool cfg_condition_failed(const struct CfgCondition *condition, const bool *fail_lid_closed);

void cfg_condition_free(struct CfgCondition *condition);

#endif // CFG_CONDITION_H
