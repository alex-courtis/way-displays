#ifndef CFG_DISABLED_H
#define CFG_DISABLED_H

#include <stdbool.h>

#include "head.h"

struct CfgDisabled {
	char *name_desc;
	const struct Pset *conditions;
};

struct CfgDisabled *cfg_disabled_init(void);

const struct Pset *cfg_disabled_pset_init(void);

const struct CfgDisabled *cfg_disabled_clone(const struct CfgDisabled * const from);

void cfg_disabled_free(struct CfgDisabled *disabled);

// name_desc must match, if conditions are present at least one must be true
bool cfg_disabled_matches_head(const struct CfgDisabled * const disabled, const struct Head * const head);

// name_desc only
bool cfg_disabled_name_desc_matches_head(const struct CfgDisabled * const disabled, const struct Head * const head);

// has conditions and exactly matches name_desc
bool cfg_disabled_has_conditions_and_name_desc(const struct CfgDisabled * const disabled, const char * const name_desc);

#endif // CFG_DISABLED_H
