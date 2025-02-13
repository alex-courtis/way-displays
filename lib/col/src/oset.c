#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"

#include "oset.h"

struct OSet {
	const void **vals;
	size_t capacity;
	size_t grow;
	size_t size;
};

struct OSetIterP {
	/*
	 * Public, removed const
	 */
	const void* val;

	/*
	 * Private
	 */
	const struct OSet *set;
	const void **v;
};

// grow to capacity + grow
void grow_oset(struct OSet *set) {

	// grow new arrays
	const void **new_vals = calloc(set->capacity + set->grow, sizeof(void*));

	// copy old arrays
	memcpy(new_vals, set->vals, set->capacity * sizeof(void*));

	// free old arrays
	free(set->vals);

	// lock in new
	set->vals = new_vals;
	set->capacity += set->grow;
}

const struct OSet *oset_init(const size_t initial, const size_t grow) {
	if (initial == 0 || grow == 0)
		return NULL;

	struct OSet *set = calloc(1, sizeof(struct OSet));
	set->capacity = initial;
	set->grow = grow;
	set->vals = calloc(set->capacity, sizeof(void*));

	return set;
}

void oset_free(const void* const cvset) {
	if (!cvset)
		return;

	struct OSet *set = (struct OSet*)cvset;

	free(set->vals);

	free(set);
}

void oset_free_vals(const struct OSet* const set, fn_free_val free_val) {
	if (!set)
		return;

	// loop over vals
	for (const void **v = set->vals; v < set->vals + set->capacity; v++) {
		if (*v) {
			if (free_val) {
				free_val(*v);
			} else {
				free((void*)*v);
			}
		}
	}

	oset_free(set);
}

void oset_iter_free(const struct OSetIter* const iter) {
	if (!iter)
		return;

	free((struct OSetIterP*)iter);
}

bool oset_contains(const struct OSet* const set, const void* const val) {
	if (!set || !val)
		return false;

	// loop over vals
	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (*v == val) {
			return true;
		}
	}

	return false;
}

const struct OSetIter *oset_iter(const struct OSet* const set) {
	if (!set)
		return NULL;

	// loop over vals
	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (*v) {
			struct OSetIterP *iterp = calloc(1, sizeof(struct OSetIterP));

			iterp->set = set;
			iterp->val = *v;
			iterp->v = v;

			return (struct OSetIter*)iterp;
		}
	}

	return NULL;
}

const struct OSetIter *oset_next(const struct OSetIter* const iter) {
	if (!iter)
		return NULL;

	struct OSetIterP *iterp = (struct OSetIterP*)iter;

	if (!iterp || !iterp->set) {
		oset_iter_free(iter);
		return NULL;
	}

	// loop over vals
	while (++iterp->v < iterp->set->vals + iterp->set->size) {
		if (*iterp->v) {
			iterp->val = *(iterp->v);
			return iter;
		}
	}

	oset_iter_free(iter);
	return NULL;
}

bool oset_add(const struct OSet* const cset, const void* const val) {
	if (!cset || !val)
		return false;

	struct OSet *set = (struct OSet*)cset;

	// loop over vals
	const void **v;
	for (v = set->vals; v < set->vals + set->size; v++) {

		// already present
		if (*v == val) {
			return false;
		}
	}

	// maybe grow for new entry
	if (set->size >= set->capacity) {
		grow_oset(set);
		v = &set->vals[set->size];
	}

	// new value
	*v = (void*)val;
	set->size++;

	return true;
}

bool oset_remove(const struct OSet* const cset, const void* const val) {
	if (!cset || !val)
		return false;

	struct OSet *set = (struct OSet*)cset;

	// loop over vals
	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (*v == val) {

			*v = NULL;
			set->size--;

			// shift down over removed
			const void **m;
			for (m = v; m < v + set->size; m++) {
				*m = *(m + 1);
			}
			*m = NULL;

			return true;
		}
	}

	return false;
}

bool oset_equal(const struct OSet* const a, const struct OSet* const b, fn_equals equals) {
	if (!a || !b || a->size != b->size)
		return false;

	for (const void **av = a->vals, **bv = b->vals; av < (a->vals + a->size); av++, bv++) {

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

struct SList *oset_vals_slist(const struct OSet* const set) {
	if (!set)
		return NULL;

	struct SList *list = NULL;

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		slist_append(&list, (void*)*v);
	}

	return list;
}

size_t oset_size(const struct OSet* const set) {
	return set ? set->size : 0;
}

size_t oset_capacity(const struct OSet* const set) {
	return set ? set->capacity : 0;
}
