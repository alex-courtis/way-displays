#include <stdbool.h>
#include <stdlib.h>

#include "conditions.h"

#include "lid.h"
#include "slist.h"
#include "fn.h"
#include "head.h"


void condition_free(const void *data) {
	struct Condition *condition = (struct Condition*)data;

	slist_free_vals(&condition->plugged, NULL);
	slist_free_vals(&condition->unplugged, NULL);

	free(condition);
}

void* fn_clone_condition(const void *data) {
	struct Condition *original = (struct Condition*)data;
	struct Condition *cloned = (struct Condition*)calloc(1, sizeof(struct Condition));

	cloned->plugged = slist_clone(original->plugged, fn_clone_strdup);
	cloned->unplugged = slist_clone(original->unplugged, fn_clone_strdup);
	cloned->lid = original->lid;

	return cloned;
}

bool fn_equal_condition(const void *a, const void *b) {
	struct Condition *lhs = (struct Condition*)a;
	struct Condition *rhs = (struct Condition*)b;

	return slist_equal(lhs->plugged, rhs->plugged, fn_equal_strcmp) &&
	       slist_equal(lhs->unplugged, rhs->unplugged, fn_equal_strcmp) &&
		   lhs->lid == rhs->lid;
}

bool condition_evaluate(const struct Condition *condition) {
	if (condition == NULL) {
		return true;
	}

	for (const struct SList *i = condition->plugged; i; i = i->nex) {
		const char* name_desc = (const char*)i->val;
		if (slist_find_equal(g_heads, head_matches_name_desc, name_desc) == NULL) {
			return false;
		}
	}

	for (const struct SList *i = condition->unplugged; i; i = i->nex) {
		const char* name_desc = (const char*)i->val;
		if (slist_find_equal(g_heads, head_matches_name_desc, name_desc) != NULL) {
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

bool condition_list_evaluate(const struct SList *conditions) {
	for (const struct SList *i = conditions; i; i = i->nex) {
		const struct Condition* condition = (struct Condition*)i->val;

		if (!condition_evaluate(condition)) {
			return false;
		}
	}

	return true;
}
