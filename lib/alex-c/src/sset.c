#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "pset.h"

#include "sset.h"

struct SSet {
	const struct SSetParams params;
	const struct PSet *pset;
};

struct SSetItState {
	const struct PSetIt *pit;
};

static const struct SSetIt *it_init(const struct PSetIt *pit) {
	if (!pit)
		return NULL;

	struct SSetIt *it = calloc(1, sizeof(struct SSetIt));
	it->st = calloc(1, sizeof(struct SSetItState));

	it->st->pit = pit;
	it->val = pit->val;

	return it;
}

const struct SSet *sset_init(void) {
	const struct SSetParams params = { 0 };
	return sset_init_with(params);
}

const struct SSet *sset_init_with(const struct SSetParams params) {
	const struct PSetParams pset_params = {
		.equal_val = params.case_insensitive ? (fn_equal)fn_equal_strcasecmp : (fn_equal)fn_equal_strcmp,
		.alloc_val = fn_clone_strdup,
		.free_val = (fn_free)free,
		.clone_val = fn_clone_strdup,
		.str_val = (fn_str)fn_str_or_null,
		.initial = params.initial,
		.grow = params.grow,
	};

	struct SSet *set = calloc(1, sizeof(struct SSet));
	set->pset = pset_init_with(pset_params);;
	memcpy((void*)&set->params, &params, sizeof(struct SSetParams));

	return set;
}

const struct SSet *sset_clone(const struct SSet* const from) {
	if (!from)
		return NULL;

	struct SSet *to = calloc(1, sizeof(struct SSet));
	to->pset = pset_clone_deep(from->pset);
	memcpy((void*)&to->params, &from->params, sizeof(struct SSetParams));

	return to;
}

void sset_free(const struct SSet* const set) {
	if (!set)
		return;

	pset_free_vals(set->pset);

	free((void*)set);
}

void sset_it_free(const struct SSetIt* const it) {
	if (!it)
		return;

	if (it->st)
		pset_it_free(it->st->pit);

	free((void*)it->st);
	free((void*)it);
}

bool sset_contains(const struct SSet* const set, const char* const val) {
	return set ? pset_contains(set->pset, val) : false;
}

const void *sset_match(const struct SSet* const set, fn_match_sset match, const void* const data) {
	return set ? pset_match(set->pset, (fn_match_val)match, data) : NULL;
}

const struct SSetIt *sset_it(const struct SSet* const set) {
	return set ? it_init(pset_it(set->pset)) : NULL;
}

const struct SSetIt *sset_match_it(const struct SSet* const set, fn_match_sset match, const void* const data) {
	return set ? it_init(pset_match_it(set->pset, (fn_match_val)match, data)) : NULL;
}

const struct SSetIt *sset_it_next(const struct SSetIt* const it) {
	if (!it)
		return NULL;


	if (!it->st) {
		sset_it_free(it);
		return NULL;
	}

	it->st->pit = pset_it_next(it->st->pit);

	if (it->st->pit) {
		struct SSetIt *it_m = (struct SSetIt*)it;
		it_m->val = it->st->pit->val;
		return it;
	} else {
		sset_it_free(it);
		return NULL;
	}
}

bool sset_add(const struct SSet* const set, const char* const val) {
	return set ? pset_add(set->pset, val) : false;
}

bool sset_remove(const struct SSet* const set, const char* const val) {
	return set ? pset_remove_free(set->pset, val) : false;
}

void sset_sort(const struct SSet* const set) {
	if (set)
		pset_sort(set->pset, set->params.case_insensitive ? (fn_equal)fn_less_than_strcasecmp : (fn_equal)fn_less_than_strcmp);
}

bool sset_equal(const struct SSet* const a, const struct SSet* const b) {
	return a && b ? pset_equal(a->pset, b->pset) : false;
}

struct SList *sset_slist_deep(const struct SSet* const set) {
	return set ? pset_slist_deep(set->pset) : NULL;
}

char *sset_str(const struct SSet* const set) {
	return set ? pset_str(set->pset) : NULL;
}

size_t sset_size(const struct SSet* const set) {
	return set ? pset_size(set->pset) : 0;
}
