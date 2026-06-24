#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cfg/disabled.h"

#include "cfg.h"
#include "conditions.h"
#include "fn.h"
#include "pset.h"
#include "slist.h"

static bool disabled_equal(const struct Disabled* const a, const struct Disabled* const b) {
	if (!a || !b) {
		return false;
	}

	if (!a->name_desc || !b->name_desc) {
		return false;
	}

	if (strcmp(a->name_desc, b->name_desc) != 0) {
		return false;
	}

	return slist_equal(a->conditions, b->conditions, fn_equal_condition);
}

struct Disabled *disabled_init_always(const char *name_desc) {
	struct Disabled *d = calloc(1, sizeof(struct Disabled));

	d->name_desc = strdup(name_desc);
	d->conditions = NULL;

	return d;
}

const struct PSet *disabled_pset_init(void) {
	const struct PSetParams params = {
		.equal_val = (fn_equal)disabled_equal,
		.free_val = (fn_free)disabled_free,
		.clone_val = (fn_clone)disabled_clone,
	};
	return pset_init_with(params);
}

const struct Disabled *disabled_clone(const struct Disabled * const from) {
	if (!from)
		return NULL;

	struct Disabled *to = (struct Disabled*)calloc(1, sizeof(struct Disabled));

	to->name_desc = strdup(from->name_desc);
	to->conditions = slist_clone(from->conditions, fn_clone_condition);

	return to;
}

void disabled_free(struct Disabled *disabled) {
	if (!disabled)
		return;

	free(disabled->name_desc);

	slist_free_vals(&disabled->conditions, condition_free);

	free(disabled);
}
