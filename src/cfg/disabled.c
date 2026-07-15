#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cfg/disabled.h"

#include "cfg/condition.h"
#include "fn.h"
#include "head.h"
#include "pset.h"

static bool disabled_equal(const struct CfgDisabled* const a, const struct CfgDisabled* const b) {
	if (!a || !b) {
		return false;
	}

	if (!a->name_desc || !b->name_desc) {
		return false;
	}

	if (strcmp(a->name_desc, b->name_desc) != 0) {
		return false;
	}

	return pset_equal(a->conditions, b->conditions);
}

struct CfgDisabled *cfg_disabled_init(void) {
	struct CfgDisabled *d = calloc(1, sizeof(struct CfgDisabled));

	d->conditions = cfg_condition_pset_init();

	return d;
}

const struct Pset *cfg_disabled_pset_init(void) {
	const struct PsetParams params = {
		.equal_val = (fn_equal)disabled_equal,
		.free_val = (fn_free)cfg_disabled_free,
		.clone_val = (fn_clone)cfg_disabled_clone,
	};
	return pset_init_with(params);
}

const struct CfgDisabled *cfg_disabled_clone(const struct CfgDisabled * const from) {
	if (!from)
		return NULL;

	struct CfgDisabled *to = (struct CfgDisabled*)calloc(1, sizeof(struct CfgDisabled));

	if (from->name_desc) {
		to->name_desc = strdup(from->name_desc);
	}

	to->conditions = pset_clone_deep(from->conditions);

	return to;
}

void cfg_disabled_free(struct CfgDisabled *disabled) {
	if (!disabled)
		return;

	free(disabled->name_desc);

	pset_free_vals(disabled->conditions);

	free(disabled);
}

bool cfg_disabled_matches_head(const struct CfgDisabled * const disabled, const struct Head * const head) {
	return
		// name_desc must match
		cfg_disabled_name_desc_matches_head(disabled, head) &&
		// all conditions must be false
		pset_find(disabled->conditions, (fn_2pred)cfg_condition_true, NULL) == NULL;
}

bool cfg_disabled_name_desc_matches_head(const struct CfgDisabled * const disabled, const struct Head * const head) {
	return disabled && head && head_matches_name_desc(head, disabled->name_desc);
}

bool cfg_disabled_has_conditions_and_name_desc(const struct CfgDisabled * const disabled, const char * const name_desc) {
	if (!disabled)
		return false;

	return (strcmp(disabled->name_desc, name_desc) == 0) && pset_size(disabled->conditions) > 0;
}

