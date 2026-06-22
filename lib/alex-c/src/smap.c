#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "pmap.h"

#include "smap.h"

struct SMap {
	const struct SMapParams params;
	const struct PMap *ptab;
};

struct SMapIterState {
	const struct PMapIter *pit;
};

const struct SMap *smap_init(void) {
	const struct SMapParams params = { 0 };
	return smap_init_with(params);
}

static const struct SMap *clone(const struct SMap* const from, bool deep) {
	if (!from)
		return NULL;

	struct SMap *to = calloc(1, sizeof(struct SMap));

	to->ptab = deep ? pmap_clone_deep(from->ptab) : pmap_clone_shallow(from->ptab) ;

	memcpy((void*)&to->params, &from->params, sizeof(struct SMapParams));

	return to;
}

const struct SMap *smap_init_with(const struct SMapParams params) {
	const struct PMapParams pmap_params = {
		.equal_key = params.case_insensitive ? fn_equal_strcasecmp : fn_equal_strcmp,
		.equal_val = params.equal_val,
		.alloc_key = fn_clone_strdup,
		.alloc_val = params.alloc_val,
		.free_key = (fn_free)free,
		.free_val = params.free_val,
		.clone_val = params.clone_val,
		.str_key = fn_str_or_null,
		.str_val = params.str_val,
		.initial = params.initial,
		.grow = params.grow,
	};

	struct SMap *tab =  calloc(1, sizeof(struct SMap));
	tab->ptab = pmap_init_with(pmap_params);;
	memcpy((void*)&tab->params, &params, sizeof(struct SMapParams));

	return tab;
}

const struct SMap *smap_clone_shallow(const struct SMap* const from) {
	return clone(from, false);
}

const struct SMap *smap_clone_deep(const struct SMap* const from) {
	return clone(from, true);
}

void smap_free(const struct SMap* const tab) {
	if (!tab)
		return;

	pmap_free(tab->ptab);

	free((void*)tab);
}

void smap_free_vals(const struct SMap* const tab) {
	if (!tab)
		return;

	pmap_free_vals(tab->ptab);

	free((void*)tab);
}

void smap_iter_free(const struct SMapIter* const iter) {
	if (!iter)
		return;

	if (iter->st) {
		pmap_iter_free(iter->st->pit);
	}

	free(iter->st);
	free((void*)iter);
}

const void *smap_get(const struct SMap* const tab, const char* const key) {
	return tab ? pmap_get(tab->ptab, key) : NULL;
}

bool smap_contains_key(const struct SMap* const tab, const char* const key) {
	return tab ? pmap_contains_key(tab->ptab, key) : false;
}

const struct SMapIter *smap_iter(const struct SMap* const tab) {
	return smap_filter_iter(tab, NULL, NULL, NULL);
}

const struct SMapIter *smap_filter_iter(const struct SMap* const tab, fn_equal equal_key, fn_equal equal_val, const void* const data) {
	if (!tab)
		return NULL;

	const struct PMapIter *pit = pmap_filter_iter(tab->ptab, equal_key, equal_val, data);

	if (!pit)
		return NULL;

	struct SMapIter *it = calloc(1, sizeof(struct SMapIter));
	it->st = calloc(1, sizeof(struct SMapIterState));

	it->st->pit = pit;
	it->key = pit->key;
	it->val = pit->val;

	return it;
}

const struct SMapIter *smap_iter_next(const struct SMapIter* const citer) {
	if (!citer)
		return NULL;

	struct SMapIter *iter = (struct SMapIter*)citer;

	if (!iter->st) {
		smap_iter_free(iter);
		return NULL;
	}

	iter->st->pit = pmap_iter_next(citer->st->pit);

	if (iter->st->pit) {
		iter->key = iter->st->pit->key;
		iter->val = iter->st->pit->val;
		return iter;
	} else {
		smap_iter_free(iter);
		return NULL;
	}
}

const void *smap_put(const struct SMap* const tab, const char* const key, const void* const val) {
	return tab ? pmap_put(tab->ptab, key, val) : NULL;
}

const void *smap_put_if_absent(const struct SMap* const tab, const char* const key, const void* const val) {
	return tab ? pmap_put_if_absent(tab->ptab, key, val) : NULL;
}

bool smap_put_free(const struct SMap* const tab, const  char* const key, const void* const val) {
	return tab ? pmap_put_free(tab->ptab, key, val) : false;
}

const void *smap_remove(const struct SMap* const tab, const char* const key) {
	return tab ? pmap_remove(tab->ptab, key) : NULL;
}

bool smap_remove_free(const struct SMap* const tab, const char* const key) {
	return tab ? pmap_remove_free(tab->ptab, key) : false;
}

bool smap_equal(const struct SMap* const a, const struct SMap* const b) {
	return a && b ? pmap_equal(a->ptab, b->ptab) : false;
}

struct SList *smap_keys_slist_deep(const struct SMap* const tab) {
	return tab ? pmap_keys_slist_deep(tab->ptab) : NULL;
}

struct SList *smap_vals_slist_shallow(const struct SMap* const tab) {
	return tab ? pmap_vals_slist_shallow(tab->ptab) : NULL;
}

struct SList *smap_vals_slist_deep(const struct SMap* const tab) {
	return tab ? pmap_vals_slist_deep(tab->ptab) : NULL;
}

char *smap_str(const struct SMap* const tab) {
	return tab ? pmap_str(tab->ptab) : NULL;
}

size_t smap_size(const struct SMap* const tab) {
	return tab ? pmap_size(tab->ptab) : 0;
}
