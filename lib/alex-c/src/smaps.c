#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "pmap.h"

#include "smaps.h"

struct SMapS {
	const struct SMapSParams params;
	const struct PMap *ptab;
};

struct SMapSIterState {
	const struct PMapIter *pit;
};

const struct SMapS *smaps_init(void) {
	const struct SMapSParams params = { 0 };
	return smaps_init_with(params);
}

const struct SMapS *smaps_init_with(const struct SMapSParams params) {
	const struct PMapParams pmap_params = {
		.equal_key = params.case_insensitive_key ? fn_equal_strcasecmp : fn_equal_strcmp,
		.equal_val = params.case_insensitive_val ? fn_equal_strcasecmp : fn_equal_strcmp,
		.alloc_key = fn_clone_strdup,
		.alloc_val = fn_clone_strdup,
		.free_key = (fn_free)free,
		.free_val = (fn_free)free,
		.clone_val = fn_clone_strdup,
		.str_key = fn_str_or_null,
		.str_val = fn_str_or_null,
		.initial = params.initial,
		.grow = params.grow,
	};

	struct SMapS *tab = calloc(1, sizeof(struct SMapS));
	tab->ptab = pmap_init_with(pmap_params);;
	memcpy((void*)&tab->params, &params, sizeof(struct SMapSParams));

	return tab;
}

const struct SMapS *smaps_clone(const struct SMapS* const from) {
	if (!from)
		return NULL;

	struct SMapS *to = calloc(1, sizeof(struct SMapS));
	to->ptab = pmap_clone_deep(from->ptab);
	memcpy((void*)&to->params, &from->params, sizeof(struct SMapSParams));

	return to;
}

void smaps_free(const struct SMapS* const tab) {
	if (!tab)
		return;

	pmap_free_vals(tab->ptab);

	free((void*)tab);
}

void smaps_iter_free(const struct SMapSIter* const iter) {
	if (!iter)
		return;

	if (iter->st) {
		pmap_iter_free(iter->st->pit);
	}

	free(iter->st);
	free((void*)iter);
}

const char *smaps_get(const struct SMapS* const tab, const char* const key) {
	return tab ? pmap_get(tab->ptab, key) : NULL;
}

bool smaps_contains_key(const struct SMapS* const tab, const char* const key) {
	return tab ? pmap_contains_key(tab->ptab, key) : false;
}

const struct SMapSIter *smaps_iter(const struct SMapS* const tab) {
	return smaps_filter_iter(tab, NULL, NULL, NULL);
}

const struct SMapSIter *smaps_filter_iter(const struct SMapS* const tab, fn_equal equal_key, fn_equal equal_val, const void* const data) {
	if (!tab)
		return NULL;

	const struct PMapIter *pit = pmap_filter_iter(tab->ptab, equal_key, equal_val, data);

	if (!pit)
		return NULL;

	struct SMapSIter *it = calloc(1, sizeof(struct SMapSIter));
	it->st = calloc(1, sizeof(struct SMapSIterState));

	it->st->pit = pit;
	it->key = pit->key;
	it->val = pit->val;

	return it;
}

const struct SMapSIter *smaps_iter_next(const struct SMapSIter* const citer) {
	if (!citer)
		return NULL;

	struct SMapSIter *iter = (struct SMapSIter*)citer;

	if (!iter->st) {
		smaps_iter_free(iter);
		return NULL;
	}

	iter->st->pit = pmap_iter_next(citer->st->pit);

	if (iter->st->pit) {
		iter->key = iter->st->pit->key;
		iter->val = iter->st->pit->val;
		return iter;
	} else {
		smaps_iter_free(iter);
		return NULL;
	}
}

bool smaps_put(const struct SMapS* const tab, const char* const key, const char* const val) {
	return tab ? pmap_put_free(tab->ptab, key, val): false;
}

bool smaps_put_if_absent(const struct SMapS* const tab, const char* const key, const char* const val) {
	return tab ? pmap_put_if_absent(tab->ptab, key, val) : false;
}

bool smaps_remove(const struct SMapS* const tab, const char* const key) {
	return tab ? pmap_remove_free(tab->ptab, key) : false;
}

bool smaps_equal(const struct SMapS* const a, const struct SMapS* const b) {
	return a && b ? pmap_equal(a->ptab, b->ptab) : false;
}

struct SList *smaps_keys_slist_deep(const struct SMapS* const tab) {
	return tab ? pmap_keys_slist_deep(tab->ptab) : NULL;
}

struct SList *smaps_vals_slist_deep(const struct SMapS* const tab) {
	return tab ? pmap_vals_slist_deep(tab->ptab) : NULL;
}

char *smaps_str(const struct SMapS* const tab) {
	return tab ? pmap_str(tab->ptab) : NULL;
}

size_t smaps_size(const struct SMapS* const tab) {
	return tab ? pmap_size(tab->ptab) : 0;
}
