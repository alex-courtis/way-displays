#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cfg/disabled.h"

#include "cfg.h"
#include "conditions.h"
#include "pset.h"
#include "slist.h"

static bool disabled_equal(const void *a, const void *b) {
	if (!a || !b) {
		return false;
	}

	struct Disabled *lhs = (struct Disabled*)a;
	struct Disabled *rhs = (struct Disabled*)b;

	if (!lhs->name_desc || !rhs->name_desc) {
		return false;
	}

	if (strcmp(lhs->name_desc, rhs->name_desc) != 0) {
		return false;
	}

	return slist_equal(lhs->conditions, rhs->conditions, fn_equal_condition);
}

struct Disabled *disabled_init_always(const char *name_desc) {
	struct Disabled *d = calloc(1, sizeof(struct Disabled));

	d->name_desc = strdup(name_desc);
	d->conditions = NULL;

	return d;
}

const struct PSet *disabled_pset_init(void) {
	const struct PSetParams params = {
		.equal_val = disabled_equal,
		.free_val = disabled_free,
		.clone_val = (fn_clone)disabled_clone,
	};
	return pset_init_with(params);
}

const struct Disabled *disabled_clone(const struct Disabled * const from) {
	if (!from)
		return NULL;

	struct Disabled *clone = (struct Disabled*)calloc(1, sizeof(struct Disabled));

	clone->name_desc = strdup(from->name_desc);
	clone->conditions = slist_clone(from->conditions, fn_clone_condition);

	return clone;
}

void disabled_free(const void *val) {
	struct Disabled *disabled = (struct Disabled*)val;

	if (!disabled)
		return;

	free(disabled->name_desc);

	slist_free_vals(&disabled->conditions, condition_free);

	free(disabled);
}
