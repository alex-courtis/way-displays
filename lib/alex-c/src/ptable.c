#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"
#include "str.h"

#include "ptable.h"

#define PTABLE_DEFAULT_INITIAL 10
#define PTABLE_DEFAULT_GROW 10

struct PTable {
	const struct PTableParams params;
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct PTableIterState {
	const struct PTable *tab;
	size_t position;
	fn_equal equal_key;
	fn_equal equal_val;
	const void *data;
};

// grow to capacity + grow
static void grow_ptable(struct PTable *tab) {
	size_t new_capacity = tab->capacity + (tab->params.grow ? tab->params.grow : PTABLE_DEFAULT_GROW);

	// grow new arrays
	const void **new_keys = calloc(new_capacity, sizeof(void*));
	const void **new_vals = calloc(new_capacity, sizeof(void*));

	// copy old arrays
	memcpy(new_keys, tab->keys, tab->capacity * sizeof(void*));
	memcpy(new_vals, tab->vals, tab->capacity * sizeof(void*));

	// free old arrays
	free(tab->keys);
	free(tab->vals);

	// lock in new
	tab->keys = new_keys;
	tab->vals = new_vals;
	tab->capacity = new_capacity;
}

const struct PTable *ptable_init(void) {
	const struct PTableParams params = { 0 };
	return ptable_init_with(params);
}

const struct PTable *ptable_init_with(const struct PTableParams params) {
	struct PTable *tab = calloc(1, sizeof(struct PTable));

	tab->capacity = params.initial ? params.initial : PTABLE_DEFAULT_INITIAL;
	tab->keys = calloc(tab->capacity, sizeof(void*));
	tab->vals = calloc(tab->capacity, sizeof(void*));

	memcpy((void*)&tab->params, &params, sizeof(struct PTableParams));

	return tab;
}

const struct PTable *ptable_clone(const struct PTable* const from, fn_clone clone_val) {
	if (!from)
		return NULL;

	const struct PTable *to =  ptable_init_with(from->params);

	const void **k;
	const void **v;
	for (k = from->keys, v = from->vals; k < from->keys + from->size; k++, v++) {
		ptable_put(to, *k, clone_val ? clone_val(*v) : *v);
	}

	return to;
}

void ptable_free(const struct PTable* const tab) {
	if (!tab)
		return;

	if (tab->params.free_key) {
		for (const void **k = tab->keys; k < tab->keys + tab->capacity; k++) {
			if (*k) {
				tab->params.free_key(*k);
			}
		}
	}

	free(tab->keys);
	free(tab->vals);

	free((void*)tab);
}

void ptable_free_vals(const struct PTable* const tab) {
	if (!tab)
		return;

	for (const void **v = tab->vals; v < tab->vals + tab->capacity; v++) {
		if (*v) {
			if (tab->params.free_val) {
				tab->params.free_val(*v);
			} else {
				free((void*)*v);
			}
		}
	}

	ptable_free(tab);
}

void ptable_iter_free(const struct PTableIter* const iter) {
	if (!iter)
		return;

	free(iter->st);
	free((void*)iter);
}

const void *ptable_get(const struct PTable* const tab, const void* const key) {
	if (!tab)
		return NULL;

	const void **k;
	const void **v;
	for (k = tab->keys, v = tab->vals;
			k < tab->keys + tab->size;
			k++, v++) {
		if (tab->params.equal_key ? tab->params.equal_key(*k, key) : *k == key) {
			return *v;
		}
	}

	return NULL;
}

const struct PTableIter *ptable_iter(const struct PTable* const tab) {
	return ptable_filter_iter(tab, NULL, NULL, NULL);
}

const struct PTableIter *ptable_filter_iter(const struct PTable* const tab, fn_equal equal_key, fn_equal equal_val, const void* const data) {
	if (!tab || tab->size == 0)
		return NULL;

	struct PTableIter *it = calloc(1, sizeof(struct PTableIter));
	it->st = calloc(1, sizeof(struct PTableIterState));
	it->st->tab = tab;
	it->st->equal_key = equal_key;
	it->st->equal_val = equal_val;
	it->st->data = data;

	return ptable_iter_next(it);
}

const struct PTableIter *ptable_iter_next(const struct PTableIter* const iter) {
	if (!iter)
		return NULL;

	struct PTableIter *it = (struct PTableIter*)iter;
	struct PTableIterState *st = it->st;

	if (!it->st || !it->st->tab) {
		ptable_iter_free(it);
		return NULL;
	}

	// null key indicates first use, start at the beginning
	if (it->key) {
		st->position++;
	}

	for ( ; st->position < st->tab->size; st->position++) {

		it->key = *(st->tab->keys + st->position);
		it->val = *(st->tab->vals + st->position);

		if (st->equal_key && !st->equal_key(it->key, st->data)) {
			continue;
		}
		if (st->equal_val && !st->equal_val(it->val, st->data)) {
			continue;
		}

		return it;
	}

	ptable_iter_free(it);
	return NULL;
}

const void *ptable_put(const struct PTable* const ctab, const void* const key, const void* const val) {
	if (!ctab || !key)
		return NULL;

	struct PTable *tab = (struct PTable*)ctab;

	const void **k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {

		// overwrite existing values
		if (tab->params.equal_key ? tab->params.equal_key(*k, key) : *k == key) {
			const void *prev = *v;
			*v = val;
			return prev;
		}
	}

	// grow for new entry
	if (tab->size >= tab->capacity) {
		grow_ptable(tab);
		k = &tab->keys[tab->size];
		v = &tab->vals[tab->size];
	}

	// new
	if (tab->params.alloc_key) {
		*k = tab->params.alloc_key(key);
	} else {
		*k = key;
	}
	*v = val;
	tab->size++;

	return NULL;
}

const void *ptable_remove(const struct PTable* const ctab, const void* const key) {
	if (!ctab)
		return NULL;

	struct PTable *tab = (struct PTable*)ctab;

	const void **k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {

		if (tab->params.equal_key ? tab->params.equal_key(*k, key) : *k == key) {
			if (tab->params.free_key) {
				tab->params.free_key((void*)*k);
			}
			*k = NULL;
			const void* prev = *v;
			*v = NULL;
			tab->size--;

			// shift down over removed
			const void **mk;
			const void **mv;
			for (mk = k, mv = v; mk < tab->keys + tab->size; mk++, mv++) {
				*mk = *(mk + 1);
				*mv = *(mv + 1);
			}
			*mk = NULL;
			*mv = NULL;

			return prev;
		}
	}

	return NULL;
}

bool ptable_equal(const struct PTable* const a, const struct PTable* const b) {
	if (!a || !b || a->size != b->size)
		return false;

	const void **ak, **bk;
	const void **av, **bv;

	for (ak = a->keys, bk = b->keys, av = a->vals, bv = b->vals;
			ak < a->keys + a->size;
			ak++, bk++, av++, bv++) {

		// key
		if (!(a->params.equal_key ? a->params.equal_key(*ak, *bk) : *ak == *bk)) {
			return false;
		}

		// value
		if (a->params.equal_val) {
			if (!a->params.equal_val(*av, *bv)) {
				return false;
			}
		} else if (*av != *bv) {
			return false;
		}
	}

	return true;
}

struct SList *ptable_keys_slist(const struct PTable* const tab) {
	if (!tab)
		return NULL;

	struct SList *list = NULL;

	const void **k;
	for (k = tab->keys; k < tab->keys + tab->size; k++) {
		if (tab->params.alloc_key) {
			slist_append(&list, (void*)tab->params.alloc_key(*k));
		} else {
			slist_append(&list, (void*)*k);
		}
	}

	return list;
}

struct SList *ptable_vals_slist(const struct PTable* const tab) {
	if (!tab)
		return NULL;

	struct SList *list = NULL;

	const void **k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {
		slist_append(&list, (void*)*v);
	}

	return list;
}

char *ptable_str(const struct PTable* const tab) {
	if (!tab)
		return NULL;

	char *out = strdup("");

	const void **k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {

		if (*k) {
			if (tab->params.str_key) {
				char *key = tab->params.str_key(*k);
				out = sprintf_append(out, "%s = ", key);
				free(key);
			} else {
				out = sprintf_append(out, "%p = ", *k);
			}
		} else {
			out = sprintf_append(out, "(null) = ");
		}

		if (*v) {
			if (tab->params.str_val) {
				char *val = tab->params.str_val(*v);
				out = sprintf_append(out, "%s\n", val);
				free(val);
			} else {
				out = sprintf_append(out, "%p\n", *v);
			}
		} else {
			out = sprintf_append(out, "%s", "(null)\n");
		}
	}

	return out;
}

size_t ptable_size(const struct PTable* const tab) {
	return tab ? tab->size : 0;
}
