#include <stdbool.h>
#include <stdlib.h>

#include "cfg/condition.h"

#include "displ.h"
#include "enum.h"
#include "fn.h"
#include "head.h"
#include "lid.h"
#include "ppmap.h"
#include "pset.h"
#include "sset.h"
#include "str.h"

static bool condition_equal(const struct CfgCondition* const a, const struct CfgCondition* const b) {
	return a && b && a->lid == b->lid &&
		sset_equal(a->plugged, b->plugged) &&
		sset_equal(a->unplugged, b->unplugged);
}

struct CfgCondition *cfg_condition_init(void) {
	struct CfgCondition *condition = (struct CfgCondition*)calloc(1, sizeof(struct CfgCondition));

	condition->plugged = sset_init();
	condition->unplugged = sset_init();

	return condition;
}

const struct Pset *cfg_condition_pset_init(void) {
	const struct PsetParams params = {
		.equal_val = (fn_equal)condition_equal,
		.free_val = (fn_free)cfg_condition_free,
		.clone_val = (fn_clone)cfg_condition_clone,
		.str_val = (fn_str)cfg_condition_str,
	};
	return pset_init_with(params);
}

struct CfgCondition *cfg_condition_clone(const struct CfgCondition* const from) {
	if (!from)
		return NULL;

	struct CfgCondition *to = (struct CfgCondition*)calloc(1, sizeof(struct CfgCondition));

	to->plugged = sset_clone(from->plugged);
	to->unplugged = sset_clone(from->unplugged);
	to->lid = from->lid;

	return to;
}

void cfg_condition_free(struct CfgCondition *condition) {
	if (!condition)
		return;

	sset_free(condition->plugged);
	sset_free(condition->unplugged);

	free(condition);
}

bool cfg_condition_failed(const struct CfgCondition *condition, const bool *fail_lid_closed) {
	if (!condition)
		return false;

	struct PPmapFilter f = { .val_data = (fn_pred_pp)head_matches_name_desc };

	for (const struct SsetIt *it = sset_it(condition->plugged); it; it = sset_it_next(it)) {
		f.data = it->val;
		if (!ppmap_find(g_displ->heads, f).val) {
			sset_it_free(it);
			return true;
		}
	}

	for (const struct SsetIt *it = sset_it(condition->unplugged); it; it = sset_it_next(it)) {
		f.data = it->val;
		if (ppmap_find(g_displ->heads, f).val) {
			sset_it_free(it);
			return true;
		}
	}

	switch (condition->lid) {
		case LID_CLOSED:
			if ((fail_lid_closed && *fail_lid_closed) || !g_lid || !g_lid->closed) {
				return true;
			}
			break;
		case LID_OPEN:
			if (!g_lid || g_lid->closed) {
				return true;
			}
			break;
		case LID_NOT_PRESENT:
			if (g_lid) {
				return true;
			}
			break;
		default:
			break;
	}

	return false;
}

bool cfg_condition_met(const struct CfgCondition *condition, const bool *fail_lid_closed) {
	return !cfg_condition_failed(condition, fail_lid_closed);
}

char *cfg_condition_str(const struct CfgCondition *condition) {
	if (!condition)
		return NULL;

	char *str = NULL;

	for (const struct SsetIt *it = sset_it(condition->plugged); it; it = sset_it_next(it)) {
		str = sprintf_append(str, "%s%s plugged", str ? " AND " : "", it->val);
	}

	for (const struct SsetIt *it = sset_it(condition->unplugged); it; it = sset_it_next(it)) {
		str = sprintf_append(str, "%s%s unplugged", str ? " AND " : "", it->val);
	}

	switch(condition->lid) {
		case LID_CLOSED:
			str = sprintf_append(str, "%slid closed", str ? " AND " : "");
			break;
		case LID_OPEN:
			str = sprintf_append(str, "%slid open", str ? " AND " : "");
			break;
		case LID_NOT_PRESENT:
			str = sprintf_append(str, "%slid not present", str ? " AND " : "");
			break;
		default:
			break;
	}

	return str;
}
