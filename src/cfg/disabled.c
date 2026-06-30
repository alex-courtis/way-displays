#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cfg/disabled.h"

#include "cfg/condition.h"
#include "fn.h"
#include "pset.h"

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

	return pset_equal(a->conditions, b->conditions);
}

struct Disabled *disabled_init(void) {
	struct Disabled *d = calloc(1, sizeof(struct Disabled));

	d->conditions = condition_pset_init();

	return d;
}

struct Disabled *disabled_init_name_desc(const char *name_desc) {
	struct Disabled *d = disabled_init();

	if (name_desc) {
		d->name_desc = strdup(name_desc);
	}

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

	if (from->name_desc) {
		to->name_desc = strdup(from->name_desc);
	}

	to->conditions = pset_clone_deep(from->conditions);

	return to;
}

void disabled_free(struct Disabled *disabled) {
	if (!disabled)
		return;

	free(disabled->name_desc);

	pset_free_vals(disabled->conditions);

	free(disabled);
}
