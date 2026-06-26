#include <stdbool.h>
#include <stdlib.h>

#include "cfg/condition.h"

#include "lid.h"
#include "slist.h"
#include "pset.h"
#include "sset.h"
#include "fn.h"
#include "head.h"

static bool condition_equal(const struct Condition* const a, const struct Condition* const b) {
	return a && b && a->lid == b->lid &&
		sset_equal(a->plugged, b->plugged) &&
		sset_equal(a->unplugged, b->unplugged);
}

struct Condition *condition_init(void) {
	struct Condition *condition = (struct Condition*)calloc(1, sizeof(struct Condition));

	condition->plugged = sset_init();
	condition->unplugged = sset_init();

	return condition;
}

const struct PSet *condition_pset_init(void) {
	const struct PSetParams params = {
		.equal_val = (fn_equal)condition_equal,
		.free_val = (fn_free)condition_free,
		.clone_val = (fn_clone)condition_clone,
	};
	return pset_init_with(params);
}

struct Condition *condition_clone(const struct Condition* const from) {
	if (!from)
		return NULL;

	struct Condition *to = (struct Condition*)calloc(1, sizeof(struct Condition));

	to->plugged = sset_clone(from->plugged);
	to->unplugged = sset_clone(from->unplugged);
	to->lid = from->lid;

	return to;
}

bool condition_evaluate(const struct Condition *condition) {
	if (!condition)
		return true;

	for (const struct SSetIt *it = sset_it(condition->plugged); it; it = sset_it_next(it)) {
		if (slist_find_equal(g_heads, (fn_equal)head_matches_name_desc, it->val) == NULL) {
			sset_it_free(it);
			return false;
		}
	}

	for (const struct SSetIt *it = sset_it(condition->unplugged); it; it = sset_it_next(it)) {
		if (slist_find_equal(g_heads, (fn_equal)head_matches_name_desc, it->val) != NULL) {
			sset_it_free(it);
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

bool condition_set_evaluate(const struct PSet* const conditions) {
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

	sset_free(condition->plugged);
	sset_free(condition->unplugged);

	free(condition);
}

