#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "conditions.h"

#include "slist.h"
#include "fn.h"
#include "head.h"


void condition_free(const void *data) {
	struct Condition *condition = (struct Condition*)data;

	slist_free_vals(&condition->plugged, NULL);
	slist_free_vals(&condition->unplugged, NULL);

	free(condition);
}

void* condition_clone(const void *data) {
	struct Condition *original = (struct Condition*)data;
	struct Condition *cloned = (struct Condition*)calloc(1, sizeof(struct Condition));

	cloned->plugged = slist_clone(original->plugged, fn_clone_strdup);
	cloned->unplugged = slist_clone(original->unplugged, fn_clone_strdup);

	return cloned;
}

bool condition_equal(const void *a, const void *b) {
	struct Condition *lhs = (struct Condition*)a;
	struct Condition *rhs = (struct Condition*)b;

	return slist_equal(lhs->plugged, rhs->plugged, fn_comp_equals_strcmp) &&
	       slist_equal(lhs->unplugged, rhs->unplugged, fn_comp_equals_strcmp);
}

bool condition_evaluate(const struct Condition *condition) {
	if (condition == NULL) {
		return true;
	}

	for (struct SList *i = condition->plugged; i; i = i->nex) {
		const char* name_desc = (const char*)i->val;
		if (slist_find_equal(heads, head_matches_name_desc_fuzzy, name_desc) == NULL) {
			return false;
		}
	}

	for (struct SList *i = condition->unplugged; i; i = i->nex) {
		const char* name_desc = (const char*)i->val;
		if (slist_find_equal(heads, head_matches_name_desc_fuzzy, name_desc) != NULL) {
			return false;
		}
	}

	return true;
}

bool condition_list_evaluate(const struct SList *conditions) {
	for (const struct SList *i = conditions; i; i = i->nex) {
		struct Condition* condition = (struct Condition*)i->val;

		if (!condition_evaluate(condition)) {
			return false;
		}
	}

	return true;
}
