#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "str.h"

#include "slist.h"

#include "stable.h"

/*
   diff -u \
   <(sed -e ' s/itable/xtable/g ; s/ITable/XTable/g ' src/itable.c) \
   <(sed -e 's/stable/xtable/g ; s/STable/XTable/g' src/stable.c)
   */

struct STable {
	const char **keys;
	const void **vals;
	size_t capacity;
	size_t grow;
	size_t size;
	fn_equal equal;
};

struct STableIter {
	const char *key;
	const void *val;
	const struct STable *tab;
	size_t position;
};

// grow to capacity + grow
static void grow_stable(struct STable *tab) {

	// grow new arrays
	const char **new_keys = calloc(tab->capacity + tab->grow, sizeof(char*));
	const void **new_vals = calloc(tab->capacity + tab->grow, sizeof(void*));

	// copy old arrays
	memcpy(new_keys, tab->keys, tab->capacity * sizeof(char*));
	memcpy(new_vals, tab->vals, tab->capacity * sizeof(void*));

	// free old arrays
	free(tab->keys);
	free(tab->vals);

	// lock in new
	tab->keys = new_keys;
	tab->vals = new_vals;
	tab->capacity += tab->grow;
}

const struct STable *stable_init(void) {
	return stable_init_with(10, 10, false);
}

const struct STable *stable_init_with(const size_t initial, const size_t grow, const bool case_insensitive) {
	if (initial == 0 || grow == 0)
		return NULL;

	struct STable *tab = calloc(1, sizeof(struct STable));
	tab->capacity = initial;
	tab->grow = grow;
	if (case_insensitive) {
		tab->equal = fn_equal_strcasecmp;
	} else {
		tab->equal = fn_equal_strcmp;
	}
	tab->keys = calloc(tab->capacity, sizeof(char*));
	tab->vals = calloc(tab->capacity, sizeof(void*));

	return tab;
}

void stable_free(const void* const cvtab) {
	if (!cvtab)
		return;

	struct STable *tab = (struct STable*)cvtab;

	for (const char **k = tab->keys; k < tab->keys + tab->capacity; k++) {
		free((void*)*k);
	}
	free(tab->keys);
	free(tab->vals);

	free(tab);
}

void stable_free_vals(const struct STable* const tab, fn_free free_val) {
	if (!tab)
		return;

	for (const void **v = tab->vals; v < tab->vals + tab->capacity; v++) {
		if (*v) {
			if (free_val) {
				free_val(*v);
			} else {
				free((void*)*v);
			}
		}
	}

	stable_free(tab);
}

void stable_iter_free(const struct STableIter* const iter) {
	if (!iter)
		return;

	free((void*)iter);
}

const void *stable_get(const struct STable* const tab, const char* const key) {
	if (!tab)
		return NULL;

	// loop over keys
	const char **k;
	const void **v;
	for (k = tab->keys, v = tab->vals;
			k < tab->keys + tab->size;
			k++, v++) {
		if (tab->equal(*k, key)) {
			return *v;
		}
	}

	return NULL;
}

const struct STableIter *stable_iter(const struct STable* const tab) {
	if (!tab || tab->size == 0)
		return NULL;

	// first key/val
	struct STableIter *i = calloc(1, sizeof(struct STableIter));
	i->tab = tab;
	i->key = *(tab->keys);
	i->val = *(tab->vals);
	i->position = 0;

	return i;
}

const struct STableIter *stable_iter_next(const struct STableIter* const iter) {
	if (!iter)
		return NULL;

	struct STableIter *i = (struct STableIter*)iter;

	if (!i->tab) {
		stable_iter_free(i);
		return NULL;
	}

	if (++i->position < i->tab->size) {
		i->key = *(i->tab->keys + i->position);
		i->val = *(i->tab->vals + i->position);
		return i;
	} else {
		stable_iter_free(i);
		return NULL;
	}
}

const char *stable_iter_key(const struct STableIter* const iter) {
	return iter ? iter->key : NULL;
}

const void *stable_iter_val(const struct STableIter* const iter) {
	return iter ? iter->val : NULL;
}

const void *stable_put(const struct STable* const ctab, const char* const key, const void* const val) {
	if (!ctab || !key)
		return NULL;

	struct STable *tab = (struct STable*)ctab;

	// loop over existing keys
	const char **k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {

		// overwrite existing values
		if (tab->equal(*k, key)) {
			const void *prev = *v;
			*v = val;
			return prev;
		}
	}

	// grow for new entry
	if (tab->size >= tab->capacity) {
		grow_stable(tab);
		k = &tab->keys[tab->size];
		v = &tab->vals[tab->size];
	}

	// new
	*k = strdup(key);
	*v = val;
	tab->size++;

	return NULL;
}

const void *stable_remove(const struct STable* const ctab, const char* const key) {
	if (!ctab)
		return NULL;

	struct STable *tab = (struct STable*)ctab;

	// loop over existing keys
	const char **k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {

		if (tab->equal(*k, key)) {
			free((void*)*k);
			*k = NULL;
			const void* prev = *v;
			*v = NULL;
			tab->size--;

			// shift down over removed
			const char **mk;
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

bool stable_equal(const struct STable* const a, const struct STable* const b, fn_equal equal) {
	if (!a || !b || a->size != b->size)
		return false;

	const char **ak, **bk;
	const void **av, **bv;

	for (ak = a->keys, bk = b->keys, av = a->vals, bv = b->vals;
			ak < a->keys + a->size;
			ak++, bk++, av++, bv++) {

		// key
		if (!a->equal(*ak, *bk)) {
			return false;
		}

		// value
		if (equal) {
			if (!equal(*av, *bv)) {
				return false;
			}
		} else if (*av != *bv) {
			return false;
		}
	}

	return true;
}

struct SList *stable_keys_slist(const struct STable* const tab) {
	if (!tab)
		return NULL;

	struct SList *list = NULL;

	const char **k;
	for (k = tab->keys; k < tab->keys + tab->size; k++) {
		slist_append(&list, strdup(*k));
	}

	return list;
}

struct SList *stable_vals_slist(const struct STable* const tab) {
	if (!tab)
		return NULL;

	struct SList *list = NULL;

	const char **k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {
		slist_append(&list, (void*)*v);
	}

	return list;
}

char *stable_str(const struct STable* const tab, fn_str str) {
	if (!tab)
		return NULL;

	char *out = strdup("");

	const char **k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {
		if (*v) {
			if (str) {
				char *val_str = str(*v);
				out = sprintf_append(out, "%s = %s\n", *k, val_str);
				free(val_str);
			} else {
				out = sprintf_append(out, "%s = %s\n", *k, (char*)*v);
			}
		} else {
			out = sprintf_append(out, "%s = (null)\n", *k);
		}
	}

	return out;
}

size_t stable_size(const struct STable* const tab) {
	return tab ? tab->size : 0;
}

size_t stable_capacity(const struct STable* const tab) {
	return tab ? tab->capacity : 0;
}
