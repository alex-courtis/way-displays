#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cfg/disabled.h"

#include "cfg/cfg.h"
#include "cfg/condition.h"
#include "enum.h"
#include "fn.h"
#include "head.h"
#include "log.h"
#include "pset.h"
#include "regx.h"
#include "spmap.h"

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

const struct SPmap *cfg_disabled_spmap_init(void) {
	const struct SPmapParams params = {
		.equal_val = (fn_equal)disabled_equal,
		.free_val = (fn_free)cfg_disabled_free,
		.clone_val = (fn_clone)cfg_disabled_clone,
	};
	return spmap_init_with(params);
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

// a has conditions and matches b name_desc
static bool cfg_disabled_cond_with_name_desc(const struct CfgDisabled * const a, const struct CfgDisabled * const b) {
	if (!a || !b || !a->name_desc || !b->name_desc || pset_size(a->conditions) == 0)
		return false;

	// substring match
	if (strcasestr(a->name_desc, b->name_desc) != NULL || strcasestr(b->name_desc, a->name_desc) != NULL)
		return true;

	// b matches regex a
	if (strlen(a->name_desc) > 2 && a->name_desc[0] == '!' && regex_matches(b->name_desc, a->name_desc + 1))
		return true;

	// a matches regex b
	if (strlen(b->name_desc) > 2 && b->name_desc[0] == '!' && regex_matches(a->name_desc, b->name_desc + 1))
		return true;

	return false;
}

void cfg_disabled_filter_conditional_clashes(const struct SPmap *disableds) {
	for (const struct SPmapIt *it = spmap_it(disableds); it; it = spmap_it_next(it)) {

		// current global conditionally disabled that match the name_desc
		const struct SPmapFilter f = { .val_data = (fn_pred_pp)cfg_disabled_cond_with_name_desc, .data = it->val, };
		const struct CfgDisabled *contitionally = spmap_find(g_cfg->disableds, f).val;
		if (contitionally) {

			log_info(NULL);
			log_info("Ignoring %s for '%s' as it is conditionally %s '%s'",
					cfg_element_name(DISABLED),
					((const struct CfgDisabled*)it->val)->name_desc,
					cfg_element_name(DISABLED),
					contitionally->name_desc
					);

			spmap_it_remove_free(it);
		}
	}
}

bool cfg_disabled_applies_to_head(const struct CfgDisabled * const disabled, const struct Head * const head) {
	return
		// name_desc must match
		head_matches_name_desc(head, disabled->name_desc) &&

		// all conditions must be false
		pset_find(disabled->conditions, (struct PsetFilter){ .val = (fn_pred_p)cfg_condition_true, }) == NULL;
}

bool cfg_disabled_conditionally_for_head(const struct CfgDisabled * const disabled, const struct Head * const head) {
	return disabled && head && pset_size(disabled->conditions) > 0 && head_matches_name_desc(head, disabled->name_desc);
}

