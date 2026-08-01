#ifndef CFG_DISABLED_H
#define CFG_DISABLED_H

#include <stdbool.h>

#include "head.h"
#include "spmap.h"

struct CfgDisabled {
	char *name_desc;
	const struct Pset *conditions;
};

struct CfgDisabled *cfg_disabled_init(void);

const struct SPmap *cfg_disabled_spmap_init(void);

const struct CfgDisabled *cfg_disabled_clone(const struct CfgDisabled * const from);

void cfg_disabled_free(struct CfgDisabled *disabled);

// remove any disableds with the same name as a cfg disabled
void cfg_disabled_filter_conditional_clashes(const struct SPmap *disableds);

// name_desc must match, if conditions are present at least one must be true
bool cfg_disabled_applies_to_head(const struct CfgDisabled * const disabled, const struct Head * const head);

// disabled has conditions and may apply to head
bool cfg_disabled_conditionally_for_head(const struct CfgDisabled * const disabled, const struct Head * const head);

#endif // CFG_DISABLED_H
