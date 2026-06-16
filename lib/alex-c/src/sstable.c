#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "ptable.h"

#include "sstable.h"

/*
   diff --color=always -U 10000 <(sed -e ' s/ptable/xtable/g ; s/PTable/XTable/g ' inc/ptable.h) <(sed -e 's/stable/xtable/g ; s/STable/XTable/g' inc/stable.h) | less

   diff --color=always -U 10000 <(sed -e ' s/stable/xtable/g ; s/STable/XTable/g ' inc/stable.h) <(sed -e 's/itable/xtable/g ; s/ITable/XTable/g' inc/itable.h) | less

   diff --color=always -U 10000 <(sed -e ' s/itable/xtable/g ; s/ITable/XTable/g ' src/itable.c) <(sed -e 's/stable/xtable/g ; s/STable/XTable/g' src/stable.c) | less

   diff --color=always -U 10000 <(sed -e ' s/sstable/xtable/g ; s/SSTable/XTable/g ' src/sstable.c) <(sed -e 's/stable/xtable/g ; s/STable/XTable/g' src/stable.c) | less
   */

struct SSTable {
	const struct SSTableParams params;
	const struct PTable *ptab;
};

struct SSTableIterState {
	const struct PTableIter *pit;
};

const struct SSTable *sstable_init(void) {
	const struct SSTableParams params = { 0 };
	return sstable_init_with(params);
}

const struct SSTable *sstable_init_with(const struct SSTableParams params) {
	const struct PTableParams ptable_params = {
		.equal_key = params.case_insensitive_key ? fn_equal_strcasecmp : fn_equal_strcmp,
		.equal_val = params.case_insensitive_val ? fn_equal_strcasecmp : fn_equal_strcmp,
		.alloc_key = (fn_alloc)strdup,
		.free_key = (fn_free)free,
		.initial = params.initial,
		.grow = params.grow,
	};

	struct SSTable *tab =  calloc(1, sizeof(struct SSTable));
	tab->ptab = ptable_init_with(ptable_params);;
	memcpy((void*)&tab->params, &params, sizeof(struct SSTableParams));

	return tab;
}

const struct SSTable *sstable_clone(const struct SSTable* const from) {
	if (!from)
		return NULL;

	struct SSTable *to = calloc(1, sizeof(struct SSTable));
	to->ptab = ptable_clone(from->ptab, (fn_clone)strdup);
	memcpy((void*)&to->params, &from->params, sizeof(struct SSTableParams));

	return to;
}

void sstable_free(const struct SSTable* const tab) {
	if (!tab)
		return;

	ptable_free_vals(tab->ptab, NULL);

	free((void*)tab);
}

void sstable_iter_free(const struct SSTableIter* const iter) {
	if (!iter)
		return;

	if (iter->st) {
		ptable_iter_free(iter->st->pit);
	}

	free(iter->st);
	free((void*)iter);
}

const void *sstable_get(const struct SSTable* const tab, const char* const key) {
	return tab ? ptable_get(tab->ptab, key) : NULL;
}

const struct SSTableIter *sstable_iter(const struct SSTable* const tab) {
	return sstable_filter_iter(tab, NULL, NULL, NULL);
}

const struct SSTableIter *sstable_filter_iter(const struct SSTable* const tab, fn_equal_str equal_key, fn_equal_str equal_val, const void* const data) {
	if (!tab)
		return NULL;

	const struct PTableIter *pit = ptable_filter_iter(tab->ptab, (fn_equal)equal_key, (fn_equal)equal_val, data);

	if (!pit)
		return NULL;

	struct SSTableIter *it = calloc(1, sizeof(struct SSTableIter));
	it->st = calloc(1, sizeof(struct SSTableIterState));

	it->st->pit = pit;
	it->key = pit->key;
	it->val = pit->val;

	return it;
}

const struct SSTableIter *sstable_iter_next(const struct SSTableIter* const iter) {
	if (!iter)
		return NULL;

	struct SSTableIter *it = (struct SSTableIter*)iter;

	if (!it->st || !it->st->pit) {
		sstable_iter_free(it);
		return NULL;
	}

	it->st->pit = ptable_iter_next(iter->st->pit);

	if (it->st->pit) {
		it->key = it->st->pit->key;
		it->val = it->st->pit->val;
	} else {
		sstable_iter_free(it);
		it = NULL;
	}

	return it;
}

bool sstable_put(const struct SSTable* const tab, const char* const key, const char* const val) {
	if (!tab)
		return false;

	const char *new = val ? strdup(val) : NULL;
	const char *old = ptable_put(tab->ptab, key, new);

	if (old) {
		free((void*)old);
		return true;
	} else {
		return false;
	}
}

bool sstable_remove(const struct SSTable* const tab, const char* const key) {
	if (!tab)
		return false;

	const char *old = ptable_remove(tab->ptab, key);

	if (old) {
		free((void*)old);
		return true;
	} else {
		return false;
	}
}

bool sstable_equal(const struct SSTable* const a, const struct SSTable* const b) {
	return a && b ? ptable_equal(a->ptab, b->ptab) : false;
}

struct SList *sstable_keys_slist(const struct SSTable* const tab) {
	return tab ? ptable_keys_slist(tab->ptab) : NULL;
}

struct SList *sstable_vals_slist(const struct SSTable* const tab) {
	return tab ? ptable_vals_slist(tab->ptab) : NULL;
}

char *sstable_str(const struct SSTable* const tab) {
	return tab ? ptable_str(tab->ptab, fn_str_or_null, fn_str_or_null) : NULL;
}

size_t sstable_size(const struct SSTable* const tab) {
	return tab ? ptable_size(tab->ptab) : 0;
}
