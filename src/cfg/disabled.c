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
	if (!a || !b)
		return false;

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

	to->conditions = pset_clone_deep(from->conditions);

	return to;
}

void cfg_disabled_free(struct CfgDisabled *disabled) {
	if (!disabled)
		return;

	pset_free_vals(disabled->conditions);

	free(disabled);
}

// fn_pred_spp: a_disabled has conditions and a name_desc matches b name_desc
static bool cfg_disabled_cond_with_name_desc(const char * const a, const struct CfgDisabled * const a_disabled, const char * const b) {
	if (!a || !b || pset_size(a_disabled->conditions) == 0)
		return false;

	// substring match
	if (strcasestr(a, b) != NULL || strcasestr(b, a) != NULL)
		return true;

	// b matches regex a
	if (strlen(a) > 2 && a[0] == '!' && regex_matches(b, a + 1))
		return true;

	// a matches regex b
	if (strlen(b) > 2 && b[0] == '!' && regex_matches(a, b + 1))
		return true;

	return false;
}

void cfg_disabled_filter_conditional_clashes(const struct SPmap *disableds) {
	for (const struct SPmapIt *it = spmap_it(disableds); it; it = spmap_it_next(it)) {

		// current global conditionally disabled that match the name_desc
		const struct SPmapFilter f = { .key_val_data = (fn_pred_spp)cfg_disabled_cond_with_name_desc, .data = it->key, };
		const struct SPmapPair conditionally = spmap_find(g_cfg->disableds, f);
		if (conditionally.val) {

			// TODO v2 add condition text
			log_info(NULL);
			log_info("Ignoring %s for '%s' as it is conditionally %s '%s'",
					cfg_element_name(DISABLED),
					it->key,
					cfg_element_name(DISABLED),
					conditionally.key
					);

			spmap_it_remove_free(it);
		}
	}
}

bool cfg_disabled_applies_to_head(const struct CfgCondition **condition_met, const struct SPmap * const disableds, const struct Head * const head, const bool fail_lid_closed) {
	*condition_met = NULL;

	// name_desc must match head
	const struct SPmapFilter f = { .key_data = (fn_pred_sp)head_name_desc_matches_head, .data = head, };
	for (const struct SPmapIt *it = spmap_filter_it(disableds, f); it; it = spmap_it_next(it)) {
		const struct CfgDisabled *disabled = it->val;

		// unconditionally disabled
		if (pset_size(disabled->conditions) == 0) {
			spmap_it_free(it);
			return true;
		}

		// one condition must be met
		*condition_met = pset_find(disabled->conditions, (struct PsetFilter){ .val_data = (fn_pred_pp)cfg_condition_met, .data = &fail_lid_closed });
		if (*condition_met) {
			spmap_it_free(it);
			return true;
		}
	}

	return false;
}

bool cfg_disabled_conditionally_for_head(const char * name_desc, const struct CfgDisabled * const disabled, const struct Head * const head) {
	return disabled && head && pset_size(disabled->conditions) > 0 && head_matches_name_desc(head, name_desc);
}

void cfg_disabled_add_lid_condition(const struct SPmap *disableds, const char *name_desc) {
	if (!disableds || !name_desc || spmap_contains_key(disableds, name_desc))
		return;

	const struct CfgDisabled *disabled = cfg_disabled_init();

	struct CfgCondition *condition = cfg_condition_init();
	condition->lid = LID_CLOSED;

	pset_add(disabled->conditions, condition);

	spmap_put(disableds, name_desc, disabled);
}

