#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "pset.h"

#include "sset.h"

struct Sset {
	const struct SsetParams params;
	const struct Pset *pset;
};

struct SsetItState {
	const struct PsetIt *pit;
};

static const struct SsetIt *it_init(const struct PsetIt *pit) {
	if (!pit)
		return NULL;

	struct SsetIt *it = calloc(1, sizeof(struct SsetIt));
	it->st = calloc(1, sizeof(struct SsetItState));

	it->st->pit = pit;
	it->val = pit->val;

	return it;
}

static struct PsetFilter pset_filter_init(const struct SsetFilter *filter) {
	const struct PsetFilter ppmap_filter = {
		.val = (fn_pred)filter->val,
		.data = filter->data,
		.val_data = (fn_2pred)filter->val_data,
	};

	return ppmap_filter;
}

const struct Sset *sset_init(void) {
	const struct SsetParams params = { 0 };
	return sset_init_with(params);
}

const struct Sset *sset_init_with(const struct SsetParams params) {
	const struct PsetParams pset_params = {
		.equal_val = params.case_insensitive ? (fn_equal)equal_strcasecmp : (fn_equal)equal_strcmp,
		.alloc_val = (fn_clone)clone_strdup,
		.free_val = free,
		.str_val = (fn_str)str_or_null,
		.initial = params.initial,
		.grow = params.grow,
	};

	struct Sset *set = calloc(1, sizeof(struct Sset));
	set->pset = pset_init_with(pset_params);;
	memcpy((void*)&set->params, &params, sizeof(struct SsetParams));

	return set;
}

const struct Sset *sset_clone(const struct Sset* const from) {
	if (!from)
		return NULL;

	struct Sset *to = calloc(1, sizeof(struct Sset));
	to->pset = pset_clone(from->pset);
	memcpy((void*)&to->params, &from->params, sizeof(struct SsetParams));

	return to;
}

void sset_free(const struct Sset* const set) {
	if (!set)
		return;

	pset_free_vals(set->pset);

	free((void*)set);
}

void sset_it_free(const struct SsetIt* const it) {
	if (!it)
		return;

	if (it->st)
		pset_it_free(it->st->pit);

	free((void*)it->st);
	free((void*)it);
}

bool sset_contains(const struct Sset* const set, const char* const val) {
	return set ? pset_contains(set->pset, val) : false;
}

const char *sset_at(const struct Sset* const set, const size_t i) {
	return set ? pset_at(set->pset, i) : NULL;
}

const void *sset_find(const struct Sset* const set, const struct SsetFilter filter) {
	return set ? pset_find(set->pset, pset_filter_init(&filter)) : NULL;
}

const struct SsetIt *sset_it(const struct Sset* const set) {
	return set ? it_init(pset_it(set->pset)) : NULL;
}

const struct SsetIt *sset_filter_it(const struct Sset* const set, const struct SsetFilter filter) {
	return set ? it_init(pset_filter_it(set->pset, pset_filter_init(&filter))) : NULL;
}

const struct SsetIt *sset_it_next(const struct SsetIt* const it) {
	if (!it)
		return NULL;

	if (!it->st) {
		sset_it_free(it);
		return NULL;
	}

	it->st->pit = pset_it_next(it->st->pit);

	if (it->st->pit) {
		struct SsetIt *it_m = (struct SsetIt*)it;
		it_m->val = it->st->pit->val;
		return it;
	} else {
		sset_it_free(it);
		return NULL;
	}
}

bool sset_add(const struct Sset* const set, const char* const val) {
	return set ? pset_add(set->pset, val) : false;
}

size_t sset_add_all(const struct Sset* const set, const struct Sset* const from) {
	return set && from ? pset_add_all(set->pset, from->pset) : 0;
}

bool sset_remove(const struct Sset* const set, const char* const val) {
	return set ? pset_remove_free(set->pset, val) : false;
}

size_t sset_remove_all(const struct Sset* const set) {
	return set ? pset_remove_all_free(set->pset) : 0;
}

size_t sset_remove_in(const struct Sset* const set, const struct Sset* const in) {
	return set && in ? pset_remove_in_free(set->pset, in->pset) : false;
}

void sset_it_remove(const struct SsetIt* const it) {
	if (!it)
		return;

	if (!it->st) {
		sset_it_free(it);
		return;
	}

	pset_it_remove_free(it->st->pit);

	((struct SsetIt*)it)->val = NULL;
}

void sset_sort(const struct Sset* const set) {
	if (set)
		pset_sort(set->pset, set->params.case_insensitive ? (fn_2pred)less_than_strcasecmp : (fn_2pred)less_than_strcmp);
}

bool sset_equal(const struct Sset* const a, const struct Sset* const b) {
	return a && b ? pset_equal(a->pset, b->pset) : false;
}

struct Pslist *sset_pslist(const struct Sset* const set) {
	return set ? pset_pslist(set->pset) : NULL;
}

char *sset_str(const struct Sset* const set) {
	return set ? pset_str(set->pset) : NULL;
}

size_t sset_size(const struct Sset* const set) {
	return set ? pset_size(set->pset) : 0;
}
