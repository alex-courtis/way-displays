#ifndef CFG_DISABLED_H
#define CFG_DISABLED_H

#include <stdbool.h>

#include "head.h"

struct Disabled {
	char *name_desc;
	const struct PSet *conditions;
};

struct Disabled *disabled_init(void);

const struct PSet *disabled_pset_init(void);

const struct Disabled *disabled_clone(const struct Disabled * const from);

void disabled_free(struct Disabled *disabled);

// name_desc must match, if conditions are present at least one must be true
bool disabled_matches_head(const struct Disabled * const disabled, const struct Head * const head);

// name_desc only
bool disabled_name_desc_matches_head(const struct Disabled * const disabled, const struct Head * const head);

#endif // CFG_DISABLED_H
