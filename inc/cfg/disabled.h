#ifndef CFG_DISABLED_H
#define CFG_DISABLED_H

#include <stdbool.h>

#include "head.h"
#include "spmap.h"

#define DISABLED_LAPTOP_DISPLAY_NAME_DESC_DEFAULT "!^eDP-[0-9]"

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

// fn_pred_spp: disabled has conditions and may apply to head
bool cfg_disabled_conditionally_for_head(const char * name_desc, const struct CfgDisabled * const disabled, const struct Head * const head);

/*
 * utility
 */

// one of disableds applies:
// name_desc must match
// if conditions are present one must be met and head.disabled_condition_desc will be set, otherwise freed and set to NULL
// fail_lid_closed directs to unconditionally fail lid closed conditions
bool cfg_disabled_applies_to_head(const struct SPmap * const disableds, struct Head * const head, const bool fail_lid_closed);

// remove any disableds with the same name as a cfg disabled
void cfg_disabled_filter_conditional_clashes(const struct SPmap *disableds);

// add the default lid closed condition for name_desc, if not present
void cfg_disabled_add_lid_condition(const struct SPmap *disableds, const char *name_desc);

#endif // CFG_DISABLED_H
