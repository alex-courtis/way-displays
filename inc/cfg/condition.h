#ifndef CFG_CONDITION_H
#define CFG_CONDITION_H

#include <stdbool.h>

#include "enum.h"

struct CfgCondition {
	const struct Sset *plugged;
	const struct Sset *unplugged;
	enum ConditionLid lid;
};

/*
 * lifecycle
 */
struct CfgCondition *cfg_condition_init(void);

const struct Pset *cfg_condition_pset_init(void);

struct CfgCondition *cfg_condition_clone(const struct CfgCondition* const from);

void cfg_condition_free(struct CfgCondition *condition);

/*
 * predicates
 */

// fn_pred_p: true if all elements of the condition are met, fail_lid_closed directs to unconditionally fail lid closed conditions
bool cfg_condition_met(const struct CfgCondition *condition, const bool *fail_lid_closed);

/*
 * to string
 */

char *cfg_condition_str(const struct CfgCondition *condition);

#endif // CFG_CONDITION_H
