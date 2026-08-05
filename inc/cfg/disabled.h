#ifndef CFG_DISABLED_H
#define CFG_DISABLED_H

#include <stdbool.h>

#include "head.h"
#include "spmap.h"

#define LID_DISABLED_NAME_DESC "!eDP-[0-9]"

struct CfgDisabled {
	const struct Pset *conditions;
};

/*
 * lifecycle
 */

struct CfgDisabled *cfg_disabled_init(void);

const struct SPmap *cfg_disabled_spmap_init(void);

const struct CfgDisabled *cfg_disabled_clone(const struct CfgDisabled * const from);

void cfg_disabled_free(struct CfgDisabled *disabled);

/*
 * predicates
 */

// fn_pred_spp: name_desc must match, if conditions are present at least one must be true
bool cfg_disabled_applies_to_head(const char * name_desc, const struct CfgDisabled * const disabled, const struct Head * const head);

// fn_pred_spp: disabled has conditions and may apply to head
bool cfg_disabled_conditionally_for_head(const char * name_desc, const struct CfgDisabled * const disabled, const struct Head * const head);

// fn_pred_spp: cfg_disabled_applies_to_head via a lid closed condition
bool cfg_disabled_applies_to_head_lid_closed(const char * name_desc, const struct CfgDisabled * const disabled, const struct Head * const head);

/*
 * utility
 */

// remove any disableds with the same name as a cfg disabled
void cfg_disabled_filter_conditional_clashes(const struct SPmap *disableds);

// add a default lid LID_DISABLED_NAME_DESC if not present
void cfg_disabled_add_lid_default(const struct SPmap *disableds);

#endif // CFG_DISABLED_H
