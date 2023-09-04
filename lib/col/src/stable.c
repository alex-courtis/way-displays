#include <stdlib.h>
#include <string.h>

#include "stable.h"

struct STable {
	const char **keys;
	const void **vals;
	size_t capacity;
	size_t grow;
	size_t size;
};

struct STableIterP {
	/*
	 * Public, removed const
	 */
	const char *key;
	const void *val;

	/*
	 * Private
	 */
	const struct STable *tab;
	const char **k;
	const void **v;
};

// grow to capacity + grow
void grow_stable(struct STable *tab) {

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

const struct STable *stable_init(const size_t initial, const size_t grow) {
	if (initial == 0 || grow == 0)
		return NULL;

	struct STable *tab = calloc(1, sizeof(struct STable));
	tab->capacity = initial;
	tab->grow = grow;
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

void stable_free_vals(const struct STable* const tab, void (*free_val)(const void* const val)) {
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
		if (strcmp(*k, key) == 0) {
			return *v;
		}
	}

	return NULL;
}

const struct STableIter *stable_iter(const struct STable* const tab) {
	if (!tab)
		return NULL;

	// loop over keys and vals
	const char **k;
	const void **v;
	for (k = tab->keys, v = tab->vals;
			v < tab->vals + tab->size && k < tab->keys + tab->size;
			k++, v++) {
		if (*v) {
			struct STableIterP *iterp = calloc(1, sizeof(struct STableIterP));

			iterp->tab = tab;
			iterp->key = *k;
			iterp->val = *v;
			iterp->k = k;
			iterp->v = v;

			return (struct STableIter*)iterp;
		}
	}

	return NULL;
}

const struct STableIter *stable_next(const struct STableIter* const iter) {
	if (!iter)
		return NULL;

	struct STableIterP *iterp = (struct STableIterP*)iter;

	if (!iterp || !iterp->tab) {
		stable_iter_free(iter);
		return NULL;
	}

	// loop over keys and vals
	while (++iterp->v < iterp->tab->vals + iterp->tab->size &&
			++iterp->k < iterp->tab->keys + iterp->tab->size) {
		if (*iterp->v) {
			iterp->key = *(iterp->k);
			iterp->val = *(iterp->v);
			return iter;
		}
	}

	stable_iter_free(iter);
	return NULL;
}

size_t stable_size(const struct STable* const tab) {
	if (!tab)
		return 0;

	return tab->size;
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
		if (strcmp(*k, key) == 0) {
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

		if (strcmp(*k, key) == 0) {
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

