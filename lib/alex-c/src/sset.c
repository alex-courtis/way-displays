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

struct SSetIterState {
	const struct PSetIter *pit;
};

const struct SSet *sset_init(void) {
	const struct SSetParams params = { 0 };
	return sset_init_with(params);
}

const struct SSet *sset_init_with(const struct SSetParams params) {
	const struct PSetParams pset_params = {
		.equal_val = params.case_insensitive ? fn_equal_strcasecmp : fn_equal_strcmp,
		.clone_val = (fn_clone)strdup,
		.free_val = (fn_free)free,
		.str_val = fn_str_or_null,
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

void sset_iter_free(const struct SSetIter* const iter) {
	if (!iter)
		return;

	if (iter->st)
		pset_iter_free(iter->st->pit);

	free((void*)iter->st);
	free((void*)iter);
}

bool sset_contains(const struct SSet* const set, const char* const val) {
	return set ? pset_contains(set->pset, val) : false;
}

const struct SSetIter *sset_iter(const struct SSet* const set) {
	return set ? sset_filter_iter(set, NULL, NULL) : NULL;
}

const struct SSetIter *sset_filter_iter(const struct SSet* const set, fn_equal equal_val, const void* const data) {
	if (!set)
		return NULL;

	const struct PSetIter *pit = pset_filter_iter(set->pset, equal_val, data);

	if (!pit)
		return NULL;

	struct SSetIter *it = calloc(1, sizeof(struct SSetIter));
	it->st = calloc(1, sizeof(struct SSetIterState));

	it->st->pit = pit;
	it->val = pit->val;

	return it;
}

const struct SSetIter *sset_iter_next(const struct SSetIter* const citer) {
	if (!citer)
		return NULL;

	struct SSetIter *iter = (struct SSetIter*)citer;

	if (!iter->st) {
		sset_iter_free(iter);
		return NULL;
	}

	iter->st->pit = pset_iter_next(citer->st->pit);

	if (iter->st->pit) {
		iter->val = iter->st->pit->val;
	} else {
		sset_iter_free(iter);
		iter = NULL;
	}

	return iter;
}

bool sset_add(const struct SSet* const set, const char* const val) {
	return set ? pset_add(set->pset, val) : false;
}

bool sset_remove(const struct SSet* const set, const char* const val) {
	return set ? pset_remove_free(set->pset, val) : false;
}

void sset_sort(const struct SSet* const set) {
	if (set)
		pset_sort(set->pset, set->params.case_insensitive ? fn_less_than_strcasecmp : fn_less_than_strcmp);
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
