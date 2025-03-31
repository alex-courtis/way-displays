#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "fn.h"
#include "slist.h"

#include "itable.h"

/*
   diff -u \
   <(sed -e ' s/itable/xtable/g ; s/ITable/XTable/g ' src/itable.c) \
   <(sed -e 's/ptable/xtable/g ; s/PTable/XTable/g' src/ptable.c)
   */

struct ITable {
	uint64_t *keys;
	const void **vals;
	size_t capacity;
	size_t grow;
	size_t size;
};

struct ITableIterP {
	/*
	 * Public, removed const
	 */
	uint64_t key;
	const void *val;

	/*
	 * Private
	 */
	const struct ITable *tab;
	const uint64_t *k;
	const void* const *v;
};

// grow to capacity + grow
static void grow_itable(struct ITable *tab) {

	// grow new arrays
	uint64_t *new_keys = calloc(tab->capacity + tab->grow, sizeof(uint64_t));
	const void **new_vals = calloc(tab->capacity + tab->grow, sizeof(void*));

	// copy old arrays
	memcpy(new_keys, tab->keys, tab->capacity * sizeof(uint64_t));
	memcpy(new_vals, tab->vals, tab->capacity * sizeof(void*));

	// free old arrays
	free(tab->keys);
	free(tab->vals);

	// lock in new
	tab->keys = new_keys;
	tab->vals = new_vals;
	tab->capacity += tab->grow;
}

const struct ITable *itable_init(const size_t initial, const size_t grow) {
	if (initial == 0 || grow == 0)
		return NULL;

	struct ITable *tab = calloc(1, sizeof(struct ITable));
	tab->capacity = initial;
	tab->grow = grow;
	tab->keys = calloc(tab->capacity, sizeof(uint64_t));
	tab->vals = calloc(tab->capacity, sizeof(void*));

	return tab;
}

void itable_free(const void* const cvtab) {
	if (!cvtab)
		return;

	struct ITable *tab = (struct ITable*)cvtab;

	free(tab->keys);
	free(tab->vals);

	free(tab);
}

void itable_free_vals(const struct ITable* const tab, fn_free_val free_val) {
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

	itable_free(tab);
}

void itable_iter_free(const struct ITableIter* const iter) {
	if (!iter)
		return;

	free((void*)iter);
}

const void *itable_get(const struct ITable* const tab, const uint64_t key) {
	if (!tab)
		return NULL;

	// loop over keys
	uint64_t *k;
	const void **v;
	for (k = tab->keys, v = tab->vals;
			k < tab->keys + tab->size;
			k++, v++) {
		if (*k == key) {
			return *v;
		}
	}

	return NULL;
}

const struct ITableIter *itable_iter(const struct ITable* const tab) {
	if (!tab)
		return NULL;

	// loop over keys and vals
	uint64_t *k;
	const void **v;
	for (k = tab->keys, v = tab->vals;
			v < tab->vals + tab->size && k < tab->keys + tab->size;
			k++, v++) {
		struct ITableIterP *iterp = calloc(1, sizeof(struct ITableIterP));

		iterp->tab = tab;
		iterp->key = *k;
		iterp->val = *v;
		iterp->k = k;
		iterp->v = v;

		return (struct ITableIter*)iterp;
	}

	return NULL;
}

const struct ITableIter *itable_next(const struct ITableIter* const iter) {
	if (!iter)
		return NULL;

	struct ITableIterP *iterp = (struct ITableIterP*)iter;

	if (!iterp || !iterp->tab) {
		itable_iter_free(iter);
		return NULL;
	}

	// loop over keys and vals
	while (++iterp->v < iterp->tab->vals + iterp->tab->size &&
			++iterp->k < iterp->tab->keys + iterp->tab->size) {
		iterp->key = *(iterp->k);
		iterp->val = *(iterp->v);
		return iter;
	}

	itable_iter_free(iter);
	return NULL;
}

const void *itable_put(const struct ITable* const ctab, const uint64_t key, const void* const val) {
	if (!ctab)
		return NULL;

	struct ITable *tab = (struct ITable*)ctab;

	// loop over existing keys
	uint64_t *k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {

		// overwrite existing values
		if (*k == key) {
			const void *prev = *v;
			*v = val;
			return prev;
		}
	}

	// grow for new entry
	if (tab->size >= tab->capacity) {
		grow_itable(tab);
		k = &tab->keys[tab->size];
		v = &tab->vals[tab->size];
	}

	// new
	*k = key;
	*v = val;
	tab->size++;

	return NULL;
}

const void *itable_remove(const struct ITable* const ctab, const uint64_t key) {
	if (!ctab)
		return NULL;

	struct ITable *tab = (struct ITable*)ctab;

	// loop over existing keys
	uint64_t *k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {

		if (*k == key) {
			const void* prev = *v;
			*v = NULL;
			tab->size--;

			// shift down over removed
			uint64_t *mk;
			const void **mv;
			for (mk = k, mv = v; mk < tab->keys + tab->size; mk++, mv++) {
				*mk = *(mk + 1);
				*mv = *(mv + 1);
			}
			*mk = 0;
			*mv = NULL;

			return prev;
		}
	}

	return NULL;
}

bool itable_equal(const struct ITable* const a, const struct ITable* const b, fn_equals equals) {
	if (!a || !b || a->size != b->size)
		return false;

	uint64_t *ak, *bk;
	const void **av, **bv;

	for (ak = a->keys, bk = b->keys, av = a->vals, bv = b->vals;
			ak < a->keys + a->size;
			ak++, bk++, av++, bv++) {

		// key
		if (*ak != *bk) {
			return false;
		}

		// value
		if (equals) {
			if (!equals(*av, *bv)) {
				return false;
			}
		} else if (*av != *bv) {
			return false;
		}
	}

	return true;
}

struct SList *itable_keys_slist(const struct ITable* const tab) {
	if (!tab)
		return NULL;

	struct SList *list = NULL;

	uint64_t *k;
	for (k = tab->keys; k < tab->keys + tab->size; k++) {
		slist_append(&list, (uint64_t*)k);
	}

	return list;
}

struct SList *itable_vals_slist(const struct ITable* const tab) {
	if (!tab)
		return NULL;

	struct SList *list = NULL;

	uint64_t *k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {
		slist_append(&list, (void*)*v);
	}

	return list;
}

char *itable_str(const struct ITable* const tab) {
	if (!tab)
		return NULL;

	size_t len = 1;

	// calculate length
	// slower but simpler than realloc, which can set off scanners/checkers
	uint64_t *k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {
		len +=
			20 +                    // longest uint64_t printed with PRIu64
			3 +                     // " = "
			(*v ? strlen(*v) : 6) + // value or "(null)"
			1;                      // "\n"
	}

	// render
	char *buf = (char*)calloc(len, sizeof(char));
	char *bufp = buf;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {
		bufp += snprintf(bufp, len - (bufp - buf), "%"PRIu64" = %s\n", *k, *v ? (char*)*v : "(null)");
	}

	// strip trailing newline
	if (bufp > buf) {
		*(bufp - 1) = '\0';
	}

	return buf;
}

size_t itable_size(const struct ITable* const tab) {
	return tab ? tab->size : 0;
}

size_t itable_capacity(const struct ITable* const tab) {
	return tab ? tab->capacity : 0;
}
