#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "ptable.h"
#include "str.h"

#include "itable.h"

/*
   diff --color=always -U 10000 <(sed -e ' s/ptable/xtable/g ; s/PTable/XTable/g ' inc/ptable.h) <(sed -e 's/itable/xtable/g ; s/ITable/XTable/g' inc/itable.h) | less

   diff --color=always -U 10000 <(sed -e ' s/itable/xtable/g ; s/ITable/XTable/g ' inc/itable.h) <(sed -e 's/stable/xtable/g ; s/STable/XTable/g' inc/stable.h) | less

   diff --color=always -U 10000 <(sed -e ' s/itable/xtable/g ; s/ITable/XTable/g ' src/itable.c) <(sed -e 's/stable/xtable/g ; s/STable/XTable/g' src/stable.c) | less

   diff --color=always -U 10000 <(sed -e ' s/sstable/xtable/g ; s/SSTable/XTable/g ' src/sstable.c) <(sed -e 's/stable/xtable/g ; s/STable/XTable/g' src/stable.c) | less
   */

struct ITable {
	const struct ITableParams params;
	const struct PTable *ptab;
};

struct ITableIterState {
	const struct PTableIter *pit;
	fn_equal_size_t equal_key;
	fn_equal equal_val;
	const void *data;
};

static bool fn_equal_key(const void* const a, const void* const b) {
	if (!a || !b)
		return false;

	return *(size_t*)a == *(size_t*)b;
}

static const void *fn_alloc_key(const void* const val) {
	size_t *out = calloc(1, sizeof(size_t));
	*out = *(size_t*)val;

	return out;
}

static char *fn_str_key(const void* const val) {
	return sprintf_alloc("%zu", *(size_t*)val);
}

const struct ITable *itable_init(void) {
	const struct ITableParams params = { 0 };
	return itable_init_with(params);
}

const struct ITable *itable_init_with(const struct ITableParams params) {
	const struct PTableParams ptable_params = {
		.equal_key = fn_equal_key,
		.equal_val = params.equal_val,
		.alloc_key = fn_alloc_key,
		.free_key = (fn_free)free,
		.initial = params.initial,
		.grow = params.grow,
	};

	struct ITable *tab =  calloc(1, sizeof(struct ITable));
	tab->ptab = ptable_init_with(ptable_params);;
	memcpy((void*)&tab->params, &params, sizeof(struct ITableParams));

	return tab;
}

const struct ITable *itable_clone(const struct ITable* const from, fn_clone clone_val) {
	if (!from)
		return NULL;

	struct ITable *to = calloc(1, sizeof(struct ITable));
	to->ptab = ptable_clone(from->ptab, clone_val);
	memcpy((void*)&to->params, &from->params, sizeof(struct ITableParams));

	return to;
}

void itable_free(const struct ITable* const tab) {
	if (!tab)
		return;

	ptable_free(tab->ptab);

	free((void*)tab);
}

void itable_free_vals(const struct ITable* const tab, fn_free free_val) {
	if (!tab)
		return;

	ptable_free_vals(tab->ptab, free_val);

	free((void*)tab);
}

void itable_iter_free(const struct ITableIter* const iter) {
	if (!iter)
		return;

	if (iter->st)
		ptable_iter_free(iter->st->pit);

	free(iter->st);
	free((void*)iter);
}

const void *itable_get(const struct ITable* const tab, const size_t key) {
	return tab ? ptable_get(tab->ptab, &key) : NULL;
}

const struct ITableIter *itable_iter(const struct ITable* const tab) {
	return itable_filter_iter(tab, NULL, NULL, NULL);
}

static bool fn_equal_key_wrapper(const void* const val, const void* const data) {
	const struct ITableIterState * const st = data;
	return st->equal_key(*(size_t*)val, st->data);
}

static bool fn_equal_val_wrapper(const void* const val, const void* const data) {
	const struct ITableIterState * const st = data;
	return st->equal_val(val, st->data);
}

const struct ITableIter *itable_filter_iter(const struct ITable* const tab, fn_equal_size_t equal_key, fn_equal equal_val, const void* const data) {
	if (!tab)
		return NULL;

	struct ITableIter *it = calloc(1, sizeof(struct ITableIter));
	it->st = calloc(1, sizeof(struct ITableIterState));
	it->st->equal_key = equal_key;
	it->st->equal_val = equal_val;
	it->st->data = data;

	// pass the ITableIterState as data, to be passed to the test wrappers
	const struct PTableIter *pit = ptable_filter_iter(tab->ptab, equal_key ? fn_equal_key_wrapper : NULL, equal_val ? fn_equal_val_wrapper : NULL, it->st);

	if (pit) {
		it->st->pit = pit;
		it->key = *(size_t*)pit->key;
		it->val = pit->val;
	} else {
		itable_iter_free(it);
		it = NULL;
	}

	return it;
}

const struct ITableIter *itable_iter_next(const struct ITableIter* const iter) {
	if (!iter)
		return NULL;

	struct ITableIter *it = (struct ITableIter*)iter;

	if (!it->st || !it->st->pit) {
		itable_iter_free(it);
		return NULL;
	}

	it->st->pit = ptable_iter_next(iter->st->pit);

	if (it->st->pit) {
		it->key = *(size_t*)it->st->pit->key;
		it->val = it->st->pit->val;
	} else {
		itable_iter_free(it);
		it = NULL;
	}

	return it;
}

const void *itable_put(const struct ITable* const tab, const size_t key, const void* const val) {
	return tab ? ptable_put(tab->ptab, &key, val) : NULL;
}

const void *itable_remove(const struct ITable* const tab, const size_t key) {
	return tab ? ptable_remove(tab->ptab, &key) : NULL;
}

bool itable_equal(const struct ITable* const a, const struct ITable* const b) {
	return a && b ? ptable_equal(a->ptab, b->ptab) : false;
}

struct SList *itable_vals_slist(const struct ITable* const tab) {
	return tab ? ptable_vals_slist(tab->ptab) : NULL;
}

char *itable_str(const struct ITable* const tab, fn_str str_val) {
	return tab ? ptable_str(tab->ptab, fn_str_key, str_val) : NULL;
}

size_t itable_size(const struct ITable* const tab) {
	return tab ? ptable_size(tab->ptab) : 0;
}
