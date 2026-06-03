#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"
#include "str.h"

#include "sset.h"

/*
   diff -u \
   <(sed -e ' s/pset/xset/g ; s/PSet/XSet/g ' src/pset.c) \
   <(sed -e 's/sset/xset/g ; s/SSet/XSet/g' src/sset.c)
   */

typedef int (*comparator)(const char *s1, const char *s2);

struct SSet {
	const char **vals;
	size_t capacity;
	size_t grow;
	size_t size;
	comparator cmp;
};

struct SSetIter {
	const char* val;
	const struct SSet *set;
	size_t position;
};

// grow to capacity + grow
static void grow_sset(struct SSet *set) {

	// grow new arrays
	const char **new_vals = calloc(set->capacity + set->grow, sizeof(void*));

	// copy old arrays
	memcpy(new_vals, set->vals, set->capacity * sizeof(void*));

	// free old arrays
	free(set->vals);

	// lock in new
	set->vals = new_vals;
	set->capacity += set->grow;
}

const struct SSet *sset_init(void) {
	return sset_init_with(10, 10, false);
}

const struct SSet *sset_init_with(const size_t initial, const size_t grow, const bool case_insensitive) {
	if (initial == 0 || grow == 0)
		return NULL;

	struct SSet *set = calloc(1, sizeof(struct SSet));
	set->capacity = initial;
	set->grow = grow;
	set->vals = calloc(set->capacity, sizeof(void*));
	if (case_insensitive) {
		set->cmp = strcasecmp;
	} else {
		set->cmp = strcmp;
	}

	return set;
}

void sset_free(const struct SSet* const set) {
	if (!set)
		return;

	// loop over vals
	for (const char **v = set->vals; v < set->vals + set->capacity; v++) {
		if (*v) {
			free((void*)*v);
		}
	}

	free(set->vals);

	free((void*)set);
}

void sset_iter_free(const struct SSetIter* const iter) {
	if (!iter)
		return;

	free((void*)iter);
}

bool sset_contains(const struct SSet* const set, const char* const val) {
	if (!set || !val)
		return false;

	// loop over vals
	for (const char **v = set->vals; v < set->vals + set->size; v++) {
		if (set->cmp(*v, val) == 0) {
			return true;
		}
	}

	return false;
}

const struct SSetIter *sset_iter(const struct SSet* const set) {
	if (!set || set->size == 0)
		return NULL;

	// first entry
	struct SSetIter *i = calloc(1, sizeof(struct SSetIter));
	i->set = set;
	i->val = *(set->vals);
	i->position = 0;

	return i;
}

const struct SSetIter *sset_iter_next(const struct SSetIter* const iter) {
	if (!iter)
		return NULL;

	struct SSetIter *i = (struct SSetIter*)iter;

	if (!i->set) {
		sset_iter_free(i);
		return NULL;
	}

	if (++i->position < i->set->size) {
		i->val = *(i->set->vals + i->position);
		return i;
	} else {
		sset_iter_free(i);
		return NULL;
	}
}

const char *sset_iter_val(const struct SSetIter* const iter) {
	return iter ? iter->val : NULL;
}

bool sset_add(const struct SSet* const cset, const char* const val) {
	if (!cset || !val)
		return false;

	struct SSet *set = (struct SSet*)cset;

	// loop over vals
	const char **v;
	for (v = set->vals; v < set->vals + set->size; v++) {

		// already present
		if (set->cmp(*v, val) == 0) {
			return false;
		}
	}

	// maybe grow for new entry
	if (set->size >= set->capacity) {
		grow_sset(set);
		v = &set->vals[set->size];
	}

	// new value
	*v = strdup(val);
	set->size++;

	return true;
}

bool sset_remove(const struct SSet* const cset, const char* const val) {
	if (!cset || !val)
		return false;

	struct SSet *set = (struct SSet*)cset;

	// loop over vals
	for (const char **v = set->vals; v < set->vals + set->size; v++) {
		if (set->cmp(*v, val) == 0 ) {

			free((void*)*v);

			*v = NULL;
			set->size--;

			// shift down over removed
			const char **m;
			for (m = v; m < v + set->size; m++) {
				*m = *(m + 1);
			}
			*m = NULL;

			return true;
		}
	}

	return false;
}

bool sset_equal(const struct SSet* const a, const struct SSet* const b) {
	if (!a || !b || a->size != b->size)
		return false;

	for (const char **av = a->vals, **bv = b->vals; av < (a->vals + a->size); av++, bv++) {

		if (a->cmp(*av, *bv) != 0) {
			return false;
		}
	}

	return true;
}

struct SList *sset_vals_slist(const struct SSet* const set) {
	if (!set)
		return NULL;

	struct SList *list = NULL;

	for (const char **v = set->vals; v < set->vals + set->size; v++) {
		slist_append(&list, (void*)*v);
	}

	return list;
}

char *sset_str(const struct SSet* const set) {
	if (!set)
		return NULL;

	char *str = strdup("");

	for (const char **v = set->vals; v < set->vals + set->size; v++) {
		str = sprintf_append(str, "%s\n", (char*)*v);
	}

	return str;
}

size_t sset_size(const struct SSet* const set) {
	return set ? set->size : 0;
}

size_t sset_capacity(const struct SSet* const set) {
	return set ? set->capacity : 0;
}
