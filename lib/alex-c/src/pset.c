#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"
#include "str.h"

#include "pset.h"

#define PSET_DEFAULT_INITIAL 10
#define PSET_DEFAULT_GROW 10

/*
   diff --color=always -U 10000 <(sed -e ' s/pset/xset/g ; s/PSet/XSet/g ' inc/pset.h) <(sed -e 's/sset/xset/g ; s/SSet/XSet/g' inc/sset.h) | less

   diff --color=always -U 10000 <(sed -e ' s/pset/xset/g ; s/PSet/XSet/g ' src/pset.c) <(sed -e 's/sset/xset/g ; s/SSet/XSet/g' src/sset.c) | less
   */

struct PSet {
	const struct PSetParams params;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct PSetIterState {
	const struct PSet *set;
	size_t pos;
	fn_equal equal_val;
	const void *data;
};

// grow to capacity + grow
static void grow_pset(struct PSet *set) {
	size_t new_capacity = set->capacity + (set->params.grow ? set->params.grow : PSET_DEFAULT_GROW);

	// grow new arrays
	const void **new_vals = calloc(new_capacity, sizeof(void*));

	// copy old arrays
	memcpy(new_vals, set->vals, set->capacity * sizeof(void*));

	// free old arrays
	free(set->vals);

	// lock in new
	set->vals = new_vals;
	set->capacity = new_capacity;
}

const struct PSet *pset_init(void) {
	const struct PSetParams params = { 0 };
	return pset_init_with(params);
}

const struct PSet *pset_init_with(const struct PSetParams params) {
	struct PSet *set = calloc(1, sizeof(struct PSet));

	set->capacity = params.initial ? params.initial : PSET_DEFAULT_INITIAL;
	set->vals = calloc(set->capacity, sizeof(void*));

	memcpy((void*)&set->params, &params, sizeof(struct PSetParams));

	return set;
}

const struct PSet *pset_clone(const struct PSet* const from, fn_clone clone_val) {
	if (!from)
		return NULL;

	const struct PSet *to = pset_init_with(from->params);

	for (const void **v = from->vals; v < from->vals + from->size; v++) {
		pset_add(to, clone_val ? clone_val(*v) : *v);
	}

	return to;
}

void pset_free(const struct PSet * const set) {
	if (!set)
		return;

	free(set->vals);

	free((void*)set);
}

void pset_free_vals(const struct PSet* const set) {
	if (!set)
		return;

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (*v) {
			if (set->params.free_val) {
				set->params.free_val(*v);
			} else {
				free((void*)*v);
			}
		}
	}

	pset_free(set);
}

void pset_iter_free(const struct PSetIter* const iter) {
	if (!iter)
		return;

	free((void*)iter->st);
	free((void*)iter);
}

bool pset_contains(const struct PSet* const set, const void* const val) {
	if (!set || !val)
		return false;

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (set->params.equal_val ? set->params.equal_val(*v, val) : *v == val) {
			return true;
		}
	}

	return false;
}

const struct PSetIter *pset_iter(const struct PSet* const set) {
	return pset_filter_iter(set, NULL, NULL);
}

const struct PSetIter *pset_filter_iter(const struct PSet* const set, fn_equal equal_val, const void* const data) {
	if (!set || set->size == 0)
		return NULL;

	struct PSetIter *it = calloc(1, sizeof(struct PSetIter));
	it->st = calloc(1, sizeof(struct PSetIterState));
	it->st->set = set;
	it->st->equal_val = equal_val;
	it->st->data = data;

	return pset_iter_next(it);
}

const struct PSetIter *pset_iter_next(const struct PSetIter* const iter) {
	if (!iter)
		return NULL;

	struct PSetIter *it = (struct PSetIter*)iter;
	struct PSetIterState *st = it->st;
	if (!st || !st->set) {
		pset_iter_free(it);
		return NULL;
	}

	// null val indicates first use, start at the beginning
	if (it->val) {
		st->pos++;
	}

	for ( ; st->pos < st->set->size; st->pos++) {

		it->val = *(st->set->vals + st->pos);

		if ((st->equal_val && !st->equal_val(it->val, st->data))) {
			continue;
		}

		return it;
	}

	pset_iter_free(it);
	return NULL;
}

bool pset_add(const struct PSet* const cset, const void* const val) {
	if (!cset || !val)
		return false;

	struct PSet *set = (struct PSet*)cset;

	const void **v;
	for (v = set->vals; v < set->vals + set->size; v++) {
		if (set->params.equal_val ? set->params.equal_val(*v, val) : *v == val) {
			return false;
		}
	}

	// maybe grow for new entry
	if (set->size >= set->capacity) {
		grow_pset(set);
		v = &set->vals[set->size];
	}

	// new value
	if (set->params.alloc_val) {
		*v = set->params.alloc_val(val);
	} else {
		*v = (void*)val;
	}
	set->size++;

	return true;
}

const void *pset_remove(const struct PSet* const cset, const void* const val) {
	if (!cset || !val)
		return NULL;

	struct PSet *set = (struct PSet*)cset;

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (set->params.equal_val ? set->params.equal_val(*v, val) : *v == val) {
			const void *removed = *v;

			*v = NULL;
			set->size--;

			// shift down over removed
			const void **m;
			for (m = v; m < v + set->size; m++) {
				*m = *(m + 1);
			}
			*m = NULL;

			return removed;
		}
	}

	return NULL;
}

void pset_sort(const struct PSet* const set, fn_less_than less_than_val) {
	if (!set || !less_than_val)
		return;

	static const size_t gaps[] = { 701, 301, 132, 57, 23, 10, 4, 1, 0 }; // Ciura gap sequence

	for (const size_t *gap = gaps; *gap > 0; gap++) {
		for (size_t i = *gap; i < set->size; i++) {
			const void *tmp = set->vals[i];
			size_t j;
			for (j = i; (j >= *gap) && less_than_val(tmp, set->vals[j - *gap]); j -= *gap) {
				set->vals[j] = set->vals[j - *gap];
			}
			set->vals[j] = tmp;
		}
	}
}

bool pset_equal(const struct PSet* const a, const struct PSet* const b) {
	if (!a || !b || a->size != b->size)
		return false;

	for (const void **av = a->vals, **bv = b->vals; av < (a->vals + a->size); av++, bv++) {
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

struct SList *pset_slist(const struct PSet* const set) {
	if (!set)
		return NULL;

	struct SList *list = NULL;

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (set->params.alloc_val) {
			slist_append(&list, (void*)set->params.alloc_val(*v));
		} else {
			slist_append(&list, (void*)*v);
		}
	}

	return list;
}

char *pset_str(const struct PSet* const set) {
	if (!set)
		return NULL;

	char *out = strdup("");

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (set->params.str_val) {
			char *val_str = set->params.str_val(*v);
			out = sprintf_append(out, "%s\n", val_str);
			free(val_str);
		} else {
			out = sprintf_append(out, "%p\n", *v);
		}
	}

	return out;
}
size_t pset_size(const struct PSet* const set) {
	return set ? set->size : 0;
}
