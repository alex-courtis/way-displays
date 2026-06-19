#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "pmap.h"
#include "str.h"

#include "imap.h"

struct IMap {
	const struct IMapParams params;
	const struct PMap *ptab;
};

struct IMapIterState {
	const struct PMapIter *pit;
	fn_equal_size_t equal_key;
	fn_equal equal_val;
	const void *data;
};

static bool fn_equal_key(const void* const a, const void* const b) {
	if (!a || !b)
		return false;

	return *(size_t*)a == *(size_t*)b;
}

static void *fn_clone_key(const void* const val) {
	size_t *out = calloc(1, sizeof(size_t));
	*out = *(size_t*)val;

	return out;
}

static char *fn_str_key(const void* const val) {
	return sprintf_alloc("%zu", *(size_t*)val);
}

static const struct IMap *clone(const struct IMap* const from, bool deep) {
	if (!from)
		return NULL;

	const struct PMap *ptab;

	if (deep) {
		ptab = pmap_clone_deep(from->ptab);
	} else {
		ptab = pmap_clone_shallow(from->ptab);
	}

	if (ptab) {
		struct IMap *to = calloc(1, sizeof(struct IMap));
		to->ptab = ptab;
		memcpy((void*)&to->params, &from->params, sizeof(struct IMapParams));
		return to;
	} else {
		return NULL;
	}
}

const struct IMap *imap_init(void) {
	const struct IMapParams params = { 0 };
	return imap_init_with(params);
}

const struct IMap *imap_init_with(const struct IMapParams params) {
	const struct PMapParams pmap_params = {
		.equal_key = fn_equal_key,
		.equal_val = params.equal_val,
		.clone_key = fn_clone_key,
		.clone_val = params.clone_val,
		.free_key = (fn_free)free,
		.free_val = params.free_val,
		.str_key = fn_str_key,
		.str_val = params.str_val,
		.initial = params.initial,
		.grow = params.grow,
	};

	struct IMap *tab =  calloc(1, sizeof(struct IMap));
	tab->ptab = pmap_init_with(pmap_params);;
	memcpy((void*)&tab->params, &params, sizeof(struct IMapParams));

	return tab;
}

const struct IMap *imap_clone_shallow(const struct IMap* const from) {
	return clone(from, false);
}

const struct IMap *imap_clone_deep(const struct IMap* const from) {
	return clone(from, true);
}

void imap_free(const struct IMap* const tab) {
	if (!tab)
		return;

	pmap_free(tab->ptab);

	free((void*)tab);
}

void imap_free_vals(const struct IMap* const tab) {
	if (!tab)
		return;

	pmap_free_vals(tab->ptab);

	free((void*)tab);
}

void imap_iter_free(const struct IMapIter* const iter) {
	if (!iter)
		return;

	if (iter->st)
		pmap_iter_free(iter->st->pit);

	free(iter->st);
	free((void*)iter);
}

const void *imap_get(const struct IMap* const tab, const size_t key) {
	return tab ? pmap_get(tab->ptab, &key) : NULL;
}

bool imap_contains_key(const struct IMap* const tab, const size_t key) {
	return tab ? pmap_contains_key(tab->ptab, &key) : false;
}

const struct IMapIter *imap_iter(const struct IMap* const tab) {
	return imap_filter_iter(tab, NULL, NULL, NULL);
}

static bool fn_equal_key_wrapper(const void* const val, const void* const data) {
	const struct IMapIterState * const st = data;
	return st->equal_key(*(size_t*)val, st->data);
}

static bool fn_equal_val_wrapper(const void* const val, const void* const data) {
	const struct IMapIterState * const st = data;
	return st->equal_val(val, st->data);
}

const struct IMapIter *imap_filter_iter(const struct IMap* const tab, fn_equal_size_t equal_key, fn_equal equal_val, const void* const data) {
	if (!tab)
		return NULL;

	struct IMapIter *it = calloc(1, sizeof(struct IMapIter));
	it->st = calloc(1, sizeof(struct IMapIterState));
	it->st->equal_key = equal_key;
	it->st->equal_val = equal_val;
	it->st->data = data;

	// pass the IMapIterState as data, to be passed to the test wrappers
	const struct PMapIter *pit = pmap_filter_iter(tab->ptab, equal_key ? fn_equal_key_wrapper : NULL, equal_val ? fn_equal_val_wrapper : NULL, it->st);

	if (pit) {
		it->st->pit = pit;
		it->key = *(size_t*)pit->key;
		it->val = pit->val;
	} else {
		imap_iter_free(it);
		it = NULL;
	}

	return it;
}

const struct IMapIter *imap_iter_next(const struct IMapIter* const citer) {
	if (!citer)
		return NULL;

	struct IMapIter *it = (struct IMapIter*)citer;

	if (!it->st) {
		imap_iter_free(it);
		return NULL;
	}

	it->st->pit = pmap_iter_next(citer->st->pit);

	if (it->st->pit) {
		it->key = *(size_t*)it->st->pit->key;
		it->val = it->st->pit->val;
		return it;
	} else {
		imap_iter_free(it);
		return NULL;
	}
}

const void *imap_put(const struct IMap* const tab, const size_t key, const void* const val) {
	return tab ? pmap_put(tab->ptab, &key, val) : NULL;
}

const void *imap_put_if_absent(const struct IMap* const tab, const size_t key, const void* const val) {
	return tab ? pmap_put_if_absent(tab->ptab, &key, val) : NULL;
}

bool imap_put_free(const struct IMap* const tab, const size_t key, const char* const val) {
	return tab ? pmap_put_free(tab->ptab, &key, val) : false;
}

const void *imap_remove(const struct IMap* const tab, const size_t key) {
	return tab ? pmap_remove(tab->ptab, &key) : NULL;
}

bool imap_remove_free(const struct IMap* const tab, const size_t key) {
	return tab ? pmap_remove_free(tab->ptab, &key) : false;
}

bool imap_equal(const struct IMap* const a, const struct IMap* const b) {
	return a && b ? pmap_equal(a->ptab, b->ptab) : false;
}

struct SList *imap_vals_slist_shallow(const struct IMap* const tab) {
	return tab ? pmap_vals_slist_shallow(tab->ptab) : NULL;
}

struct SList *imap_vals_slist_deep(const struct IMap* const tab) {
	return tab ? pmap_vals_slist_deep(tab->ptab) : NULL;
}

char *imap_str(const struct IMap* const tab) {
	return tab ? pmap_str(tab->ptab) : NULL;
}

size_t imap_size(const struct IMap* const tab) {
	return tab ? pmap_size(tab->ptab) : 0;
}
