#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "fn.h"

#include "slist.h"

#include "stable.h"

typedef int (*comparator)(const char *s1, const char *s2);

struct STable {
	const char **keys;
	const void **vals;
	size_t capacity;
	size_t grow;
	size_t size;
	comparator cmp;
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

const struct STable *stable_init(const size_t initial, const size_t grow, const bool case_insensitive) {
	if (initial == 0 || grow == 0)
		return NULL;

	struct STable *tab = calloc(1, sizeof(struct STable));
	tab->capacity = initial;
	tab->grow = grow;
	if (case_insensitive) {
		tab->cmp = strcasecmp;
	} else {
		tab->cmp = strcmp;
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

void stable_free_vals(const struct STable* const tab, fn_free_val free_val) {
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
		if (tab->cmp(*k, key) == 0) {
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
		if (tab->cmp(*k, key) == 0) {
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

		if (tab->cmp(*k, key) == 0) {
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

bool stable_equal(const struct STable* const a, const struct STable* const b, fn_equals equals) {
	if (!a || !b || a->size != b->size)
		return false;

	const char **ak, **bk;
	const void **av, **bv;

	for (ak = a->keys, bk = b->keys, av = a->vals, bv = b->vals;
			ak < a->keys + a->size;
			ak++, bk++, av++, bv++) {

		// key
		if (a->cmp == strcasecmp || b->cmp == strcasecmp) {
			if (strcasecmp(*ak, *bk) != 0) {
				return false;
			}
		} else {
			if (strcmp(*ak, *bk) != 0) {
				return false;
			}
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

struct SList *stable_keys_slist(const struct STable* const tab) {
	if (!tab)
		return NULL;

	struct SList *list = NULL;

	const char **k;
	for (k = tab->keys; k < tab->keys + tab->size; k++) {
		slist_append(&list, (void*)*k);
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

char *stable_str(const struct STable* const tab) {
	if (!tab)
		return NULL;

	size_t len = 1;

	// calculate length
	// slower but simpler than realloc, which can set off scanners/checkers
	const char **k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {
		len +=
			strlen(*k) +            // key is not null
			3 +                     // " = "
			(*v ? strlen(*v) : 6) + // value or "(null)"
			1;                      // "\n"
	}

	// render
	char *buf = (char*)calloc(len, sizeof(char));
	char *bufp = buf;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {
		bufp += snprintf(bufp, len - (bufp - buf), "%s = %s\n", *k, *v ? (char*)*v : "(null)");
	}

	// strip trailing newline
	if (bufp > buf) {
		*(bufp - 1) = '\0';
	}

	return buf;
}

size_t stable_size(const struct STable* const tab) {
	return tab ? tab->size : 0;
}

size_t stable_capacity(const struct STable* const tab) {
	return tab ? tab->capacity : 0;
}
