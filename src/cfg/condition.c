#include <stdbool.h>
#include <stdlib.h>

#include "cfg/condition.h"

#include "lid.h"
#include "slist.h"
#include "pset.h"
#include "fn.h"
#include "head.h"

static bool condition_equal(const struct Condition* const a, const struct Condition* const b) {
	return a && b &&
		slist_equal(a->plugged, b->plugged, (fn_equal)fn_equal_strcmp) &&
		slist_equal(a->unplugged, b->unplugged, (fn_equal)fn_equal_strcmp) &&
		a->lid == b->lid;
}

static struct Condition *condition_clone(const struct Condition* const from) {
	if (!from)
		return NULL;

	struct Condition *to = (struct Condition*)calloc(1, sizeof(struct Condition));

	to->plugged = slist_clone(from->plugged, fn_clone_strdup);
	to->unplugged = slist_clone(from->unplugged, fn_clone_strdup);
	to->lid = from->lid;

	return to;
}

const struct PSet *condition_pset_init(void) {
	const struct PSetParams params = {
		.equal_val = (fn_equal)condition_equal,
		.free_val = (fn_free)condition_free,
		.clone_val = (fn_clone)condition_clone,
	};
	return pset_init_with(params);
}

bool condition_evaluate(const struct Condition *condition) {
	if (!condition)
		return true;

	for (const struct SList *i = condition->plugged; i; i = i->nex) {
		const char* name_desc = (const char*)i->val;
		if (slist_find_equal(g_heads, (fn_equal)head_matches_name_desc, name_desc) == NULL) {
			return false;
		}
	}

	for (const struct SList *i = condition->unplugged; i; i = i->nex) {
		const char* name_desc = (const char*)i->val;
		if (slist_find_equal(g_heads, (fn_equal)head_matches_name_desc, name_desc) != NULL) {
			return false;
		}
	}

	switch (condition->lid) {
		case LID_CLOSED:
			if (!g_lid || !g_lid->closed) {
				return false;
			}
			break;
		case LID_OPEN:
			if (!g_lid || g_lid->closed) {
				return false;
			}
			break;
		case LID_NOT_PRESENT:
			if (g_lid) {
				return false;
			}
			break;
		default:
			break;
	}

	return true;
}

bool condition_list_evaluate(const struct PSet* const conditions) {
	for (const struct PSetIt *it = pset_it(conditions); it; it = pset_it_next(it)) {
		if (!condition_evaluate(it->val)) {
			pset_it_free(it);
			return false;
		}
	}

	return true;
}

void condition_free(struct Condition *condition) {
	if (!condition)
		return;

	slist_free_vals(&condition->plugged, NULL);
	slist_free_vals(&condition->unplugged, NULL);

	free(condition);
}

