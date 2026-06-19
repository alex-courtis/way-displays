#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"
#include "str.h"

#include "pmap.h"

#define PMAP_DEFAULT_INITIAL 10
#define PMAP_DEFAULT_GROW 10

struct PMap {
	const struct PMapParams params;
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct PMapIterState {
	const struct PMap *tab;
	size_t position;
	fn_equal equal_key;
	fn_equal equal_val;
	const void *data;
};

// grow to capacity + grow
static void grow(struct PMap *tab) {
	size_t new_capacity = tab->capacity + (tab->params.grow ? tab->params.grow : PMAP_DEFAULT_GROW);

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

static const void *put(const struct PMap* const ctab, const void* const key, const void* const val, fn_clone clone_val) {
	if (!key)
		return NULL;

	struct PMap *tab = (struct PMap*)ctab;

	const void **k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {

		// overwrite existing values
		if (tab->params.equal_key ? tab->params.equal_key(*k, key) : *k == key) {
			const void *val_old = *v;
			if (val && clone_val) {
				*v = clone_val(val);
			} else {
				*v = val;
			}
			return val_old;
		}
	}

	// grow for new entry
	if (tab->size >= tab->capacity) {
		grow(tab);
		k = &tab->keys[tab->size];
		v = &tab->vals[tab->size];
	}

	// new
	if (tab->params.clone_key) {
		*k = tab->params.clone_key(key);
	} else {
		*k = key;
	}
	if (val && clone_val) {
		*v = clone_val(val);
	} else {
		*v = val;
	}

	tab->size++;

	return NULL;
}

static const struct PMap *clone(const struct PMap* const from, fn_clone clone_val) {
	const struct PMap *to =  pmap_init_with(from->params);

	const void **k;
	const void **v;
	for (k = from->keys, v = from->vals; k < from->keys + from->size; k++, v++) {
		put(to, *k, *v, clone_val);
	}

	return to;
}

static struct SList *keys_slist(const struct PMap* const tab, fn_clone clone_key) {
	struct SList *list = NULL;

	const void **k;
	for (k = tab->keys; k < tab->keys + tab->size; k++) {
		if (clone_key) {
			slist_append(&list, (void*)clone_key(*k));
		} else {
			slist_append(&list, (void*)*k);
		}
	}

	return list;
}

static struct SList *vals_slist(const struct PMap* const tab, fn_clone clone_val) {
	struct SList *list = NULL;

	const void **k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {
		if (*v && clone_val) {
			slist_append(&list, (void*)clone_val(*v));
		} else {
			slist_append(&list, (void*)*v);
		}
	}

	return list;
}

const struct PMap *pmap_init(void) {
	const struct PMapParams params = { 0 };
	return pmap_init_with(params);
}

const struct PMap *pmap_init_with(const struct PMapParams params) {
	struct PMap *tab = calloc(1, sizeof(struct PMap));

	tab->capacity = params.initial ? params.initial : PMAP_DEFAULT_INITIAL;
	tab->keys = calloc(tab->capacity, sizeof(void*));
	tab->vals = calloc(tab->capacity, sizeof(void*));

	memcpy((void*)&tab->params, &params, sizeof(struct PMapParams));

	return tab;
}

const struct PMap *pmap_clone_shallow(const struct PMap* const from) {
	return from ? clone(from, NULL) : NULL;
}

const struct PMap *pmap_clone_deep(const struct PMap* const from) {
	if (!from || !from->params.clone_val)
		return NULL;

	return clone(from, from->params.clone_val);
}

void pmap_free(const struct PMap* const tab) {
	if (!tab)
		return;

	if (tab->params.free_key) {
		for (const void **k = tab->keys; k < tab->keys + tab->size; k++) {
			tab->params.free_key(*k);
		}
	}

	free(tab->keys);
	free(tab->vals);

	free((void*)tab);
}

void pmap_free_vals(const struct PMap* const tab) {
	if (!tab)
		return;

	for (const void **v = tab->vals; v < tab->vals + tab->size; v++) {
		if (*v) {
			if (tab->params.free_val) {
				tab->params.free_val(*v);
			} else {
				free((void*)*v);
			}
		}
	}

	pmap_free(tab);
}

void pmap_iter_free(const struct PMapIter* const iter) {
	if (!iter)
		return;

	free(iter->st);
	free((void*)iter);
}

const void *pmap_get(const struct PMap* const tab, const void* const key) {
	if (!tab || !key)
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

bool pmap_contains_key(const struct PMap* const tab, const void* const key) {
	if (!tab || !key)
		return false;

	const void **k;
	for (k = tab->keys;
			k < tab->keys + tab->size;
			k++) {
		if (tab->params.equal_key ? tab->params.equal_key(*k, key) : *k == key) {
			return true;
		}
	}

	return false;
}

const struct PMapIter *pmap_iter(const struct PMap* const tab) {
	return pmap_filter_iter(tab, NULL, NULL, NULL);
}

const struct PMapIter *pmap_filter_iter(const struct PMap* const tab, fn_equal equal_key, fn_equal equal_val, const void* const data) {
	if (!tab || tab->size == 0)
		return NULL;

	struct PMapIter *it = calloc(1, sizeof(struct PMapIter));
	it->st = calloc(1, sizeof(struct PMapIterState));
	it->st->tab = tab;
	it->st->equal_key = equal_key;
	it->st->equal_val = equal_val;
	it->st->data = data;

	return pmap_iter_next(it);
}

const struct PMapIter *pmap_iter_next(const struct PMapIter* const citer) {
	if (!citer)
		return NULL;

	struct PMapIter *iter = (struct PMapIter*)citer;
	struct PMapIterState *st = iter->st;

	if (!iter->st) {
		pmap_iter_free(iter);
		return NULL;
	}

	// null key indicates first use, start at the beginning
	if (iter->key) {
		st->position++;
	}

	for ( ; st->position < st->tab->size; st->position++) {

		iter->key = *(st->tab->keys + st->position);
		iter->val = *(st->tab->vals + st->position);

		if (st->equal_key && !st->equal_key(iter->key, st->data)) {
			continue;
		}
		if (st->equal_val && !st->equal_val(iter->val, st->data)) {
			continue;
		}

		return iter;
	}

	pmap_iter_free(iter);
	return NULL;
}


const void *pmap_put(const struct PMap* const tab, const void* const key, const void* const val) {
	return tab ? put(tab, key, val, tab->params.clone_val) : NULL;
}

const void *pmap_put_if_absent(const struct PMap* const tab, const void* const key, const void* const val) {
	if (!tab || !key)
		return NULL;

	if (pmap_contains_key(tab, key)) {
		return pmap_get(tab, key);
	} else {
		put(tab, key, val, tab->params.clone_val);
		return NULL;
	}
}

bool pmap_put_free(const struct PMap* const tab, const void* const key, const void* const val) {
	if (!tab)
		return false;

	const void *val_old = put(tab, key, val, tab->params.clone_val);

	if (val_old) {
		if (tab->params.free_val) {
			tab->params.free_val(val_old);
		} else {
			free((void*)val_old);
		}
		return true;
	} else {
		return false;
	}
}

const void *pmap_remove(const struct PMap* const ctab, const void* const key) {
	if (!ctab || !key)
		return NULL;

	struct PMap *tab = (struct PMap*)ctab;

	const void **k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {

		if (tab->params.equal_key ? tab->params.equal_key(*k, key) : *k == key) {
			if (tab->params.free_key) {
				tab->params.free_key((void*)*k);
			}
			*k = NULL;
			const void* val_old = *v;
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

			return val_old;
		}
	}

	return NULL;
}

bool pmap_remove_free(const struct PMap* const tab, const void* const key) {
	if (pmap_contains_key(tab, key)) {
		const void *removed = pmap_remove(tab, key);
		if (removed) {
			if (tab->params.free_val) {
				tab->params.free_val(removed);
			} else {
				free((void*)removed);
			}
		}
		return true;
	} else {
		return false;
	}
}

bool pmap_equal(const struct PMap* const a, const struct PMap* const b) {
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

struct SList *pmap_keys_slist_shallow(const struct PMap* const tab) {
	return tab ? keys_slist(tab, NULL) : NULL;
}

struct SList *pmap_keys_slist_deep(const struct PMap* const tab) {
	if (!tab || !tab->params.clone_key)
		return NULL;

	return keys_slist(tab, tab->params.clone_key);
}

struct SList *pmap_vals_slist_shallow(const struct PMap* const tab) {
	return tab ? vals_slist(tab, NULL) : NULL;
}

struct SList *pmap_vals_slist_deep(const struct PMap* const tab) {
	if (!tab || !tab->params.clone_val)
		return NULL;

	return vals_slist(tab, tab->params.clone_val);
}

char *pmap_str(const struct PMap* const tab) {
	if (!tab)
		return NULL;

	char *out = strdup("");

	const void **k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {

		if (*k) {
			if (tab->params.str_key) {
				char *key_old = tab->params.str_key(*k);
				out = sprintf_append(out, "%s = ", key_old);
				free(key_old);
			} else {
				out = sprintf_append(out, "%p = ", *k);
			}
		} else {
			out = sprintf_append(out, "(null) = ");
		}

		if (*v) {
			if (tab->params.str_val) {
				char *val_old = tab->params.str_val(*v);
				out = sprintf_append(out, "%s\n", val_old);
				free(val_old);
			} else {
				out = sprintf_append(out, "%p\n", *v);
			}
		} else {
			out = sprintf_append(out, "%s", "(null)\n");
		}
	}

	return out;
}

size_t pmap_size(const struct PMap* const tab) {
	return tab ? tab->size : 0;
}
