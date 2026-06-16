#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "stable.h"

#include "sstable.h"

// TODO try and make this a PTable wrapper

struct SSTable {
	const struct SSTableParams params;
	const struct STable *stab;
};

struct SSTableIterState {
	const struct STableIter *sit;
};

const struct SSTable *sstable_init(void) {
	const struct SSTableParams params = { 0 };
	return sstable_init_with(params);
}

const struct SSTable *sstable_init_with(const struct SSTableParams params) {
	const struct STableParams stable_params = {
		.case_insensitive = params.case_insensitive,
		.equal_val = fn_equal_strcmp,
		.grow = params.grow,
		.initial = params.initial,
	};

	struct SSTable *tab =  calloc(1, sizeof(struct SSTable));
	tab->stab = stable_init_with(stable_params);;
	memcpy((void*)&tab->params, &params, sizeof(struct SSTableParams));

	return tab;
}

const struct SSTable *sstable_clone(const struct SSTable* const from) {
	if (!from)
		return NULL;

	struct SSTable *to = calloc(1, sizeof(struct SSTable));
	to->stab = stable_clone(from->stab, (fn_clone)strdup);
	memcpy((void*)&to->params, &from->params, sizeof(struct SSTableParams));

	return to;
}

void sstable_free(const struct SSTable* const tab) {
	if (!tab)
		return;

	stable_free_vals(tab->stab, NULL);

	free((void*)tab);
}

void sstable_iter_free(const struct SSTableIter* const iter) {
	if (!iter)
		return;

	if (iter->st) {
		stable_iter_free(iter->st->sit);
	}

	free(iter->st);
	free((void*)iter);
}

const void *sstable_get(const struct SSTable* const tab, const char* const key) {
	return tab ? stable_get(tab->stab, key) : NULL;
}

const struct SSTableIter *sstable_iter(const struct SSTable* const tab) {
	return sstable_filter_iter(tab, NULL, NULL, NULL);
}

const struct SSTableIter *sstable_filter_iter(const struct SSTable* const tab, fn_equal_str equal_key, fn_equal_str equal_val, const void* const data) {
	if (!tab)
		return NULL;

	const struct STableIter *sit = stable_filter_iter(tab->stab, equal_key, (fn_equal)equal_val, data);

	if (!sit)
		return NULL;

	struct SSTableIter *it = calloc(1, sizeof(struct SSTableIter));
	it->st = calloc(1, sizeof(struct SSTableIterState));

	it->st->sit = sit;
	it->key = sit->key;
	it->val = sit->val;

	return it;
}

const struct SSTableIter *sstable_iter_next(const struct SSTableIter* const iter) {
	if (!iter)
		return NULL;

	struct SSTableIter *it = (struct SSTableIter*)iter;

	if (!it->st || !it->st->sit) {
		sstable_iter_free(it);
		return NULL;
	}

	it->st->sit = stable_iter_next(iter->st->sit);

	if (it->st->sit) {
		it->key = it->st->sit->key;
		it->val = it->st->sit->val;
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
	const char *old = stable_put(tab->stab, key, new);

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

	const char *old = stable_remove(tab->stab, key);

	if (old) {
		free((void*)old);
		return true;
	} else {
		return false;
	}
}

bool sstable_equal(const struct SSTable* const a, const struct SSTable* const b) {
	return a && b ? stable_equal(a->stab, b->stab) : false;
}

struct SList *sstable_keys_slist(const struct SSTable* const tab) {
	return tab ? stable_keys_slist(tab->stab) : NULL;
}

struct SList *sstable_vals_slist(const struct SSTable* const tab) {
	return tab ? stable_vals_slist(tab->stab) : NULL;
}

char *sstable_str(const struct SSTable* const tab) {
	return tab ? stable_str(tab->stab, fn_str_or_null) : NULL;
}

size_t sstable_size(const struct SSTable* const tab) {
	return tab ? stable_size(tab->stab) : 0;
}
