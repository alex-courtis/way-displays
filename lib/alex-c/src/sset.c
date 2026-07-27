#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "fn.h"
#include "pset.h"
#include "slist.h"

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
		.val = (fn_pred_p)filter->val,
		.data = filter->data,
		.val_data = (fn_pred_pp)filter->val_data,
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

const char *sset_find(const struct Sset* const set, const struct SsetFilter filter) {
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

bool sset_it_remove(const struct SsetIt* const it) {
	if (!it)
		return false;

	if (!it->st) {
		sset_it_free(it);
		return false;
	}

	((struct SsetIt*)it)->val = NULL;

	return pset_it_remove_free(it->st->pit);
}

void sset_sort(const struct Sset* const set) {
	if (set)
		pset_sort(set->pset, set->params.case_insensitive ? (fn_pred_pp)less_than_strcasecmp : (fn_pred_pp)less_than_strcmp);
}

bool sset_equal(const struct Sset* const a, const struct Sset* const b) {
	return a && b ? pset_equal(a->pset, b->pset) : false;
}

bool sset_equal_ordered(const struct Sset* const a, const struct Sset* const b) {
	return a && b ? pset_equal_ordered(a->pset, b->pset) : false;
}

const struct Slist *sset_slist(const struct Sset* const set) {
	if (!set)
		return NULL;

	const struct SlistParams params = {
		.case_insensitive = set->params.case_insensitive,
		.initial = MAX(pset_size(set->pset), set->params.initial),
		.grow = set->params.grow,
	};
	const struct Slist *list = slist_init_with(params);

	for (const struct SsetIt *it = sset_it(set); it; it = sset_it_next(it)) {
		slist_append(list, it->val);
	}

	return list;
}

char *sset_str(const struct Sset* const set) {
	return set ? pset_str(set->pset) : NULL;
}

size_t sset_size(const struct Sset* const set) {
	return set ? pset_size(set->pset) : 0;
}
