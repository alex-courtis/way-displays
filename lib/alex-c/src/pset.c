#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "pslist.h"
#include "str.h"

#include "pset.h"

#define PSET_DEFAULT_INITIAL 10
#define PSET_DEFAULT_GROW 10

struct Pset {
	const struct PsetParams params;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct PsetItState {
	const struct Pset *set;
	size_t position;
	bool attached;
	fn_2pred pred_val;
	const void *data;
};

// grow to capacity + grow
static void grow(struct Pset *set) {
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

static bool add(const struct Pset* const set, const void* const val, fn_clone alloc_val) {
	if (!val)
		return false;

	const void **v;
	for (v = set->vals; v < set->vals + set->size; v++) {
		if (set->params.equal_val ? set->params.equal_val(*v, val) : *v == val) {
			return false;
		}
	}

	struct Pset *set_m = (struct Pset*)set;

	// create new value
	const void *new = alloc_val ? alloc_val(val) : val;
	if (!new)
		return false;

	// maybe grow for new entry
	if (set->size >= set->capacity) {
		grow(set_m);
		v = &set->vals[set->size];
	}

	// assign new value
	*v = new;
	set_m->size++;

	return true;
}

static bool remove(const struct Pset* const cset, const void* const val, fn_free free_val) {
	if (!val)
		return false;

	struct Pset *set = (struct Pset*)cset;

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (set->params.equal_val ? set->params.equal_val(*v, val) : *v == val) {
			if (free_val) {
				free_val((void*)*v);
			}

			*v = NULL;
			set->size--;

			// shift down over removed
			const void **m;
			for (m = v; m < set->vals + set->size; m++) {
				*m = *(m + 1);
			}
			*m = NULL;

			return true;
		}
	}

	return false;
}

static void free_all(const struct Pset* const cset) {
	struct Pset *set = (struct Pset*)cset;

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (*v) {
			if (set->params.free_val) {
				set->params.free_val((void*)*v);
			} else {
				free((void*)*v);
			}
		}
	}
}

static void wipe_vals(const struct Pset* const cset) {
	struct Pset *set = (struct Pset*)cset;
	memset(set->vals, 0, set->size * sizeof(void*));
	set->size = 0;
}

static void it_remove(const struct PsetIt* const it, bool do_free) {
	if (!it)
		return;

	struct PsetItState *st = it->st;
	if (!st) {
		pset_it_free(it);
		return;
	}

	remove(st->set, it->val, do_free ? st->set->params.free_val: NULL);

	if (st->position > 0) {
		st->position--;
	} else {
		st->attached = false;
	}

	((struct PsetIt*)it)->val = NULL;
}

static size_t add_all(const struct Pset* const set, const struct Pset* const from, fn_clone clone_val) {
	size_t added = 0;

	for (const void **v = from->vals; v < from->vals + from->size; v++) {
		if (add(set, *v, clone_val)) {
			added++;
		}
	}

	return added;
}

static size_t remove_from(const struct Pset* const set, const struct Pset* const from, fn_free free_val) {
	size_t removed = 0;

	for (const void **v = from->vals; v < from->vals + from->size; v++) {
		if (remove(set, *v, free_val)) {
			removed++;
		}
	}

	return removed;
}

static const struct Pset *clone(const struct Pset* const from, fn_clone clone_val) {
	const struct Pset *to = pset_init_with(from->params);

	for (const void **v = from->vals; v < from->vals + from->size; v++) {
		add(to, *v, clone_val);
	}

	return to;
}

static struct Pslist *slist(const struct Pset* const set, fn_clone clone_val) {
	struct Pslist *list = NULL;

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (clone_val) {
			pslist_append(&list, (void*)clone_val(*v));
		} else {
			pslist_append(&list, (void*)*v);
		}
	}

	return list;
}

const struct Pset *pset_init(void) {
	const struct PsetParams params = { 0 };
	return pset_init_with(params);
}

const struct Pset *pset_init_with(const struct PsetParams params) {
	struct Pset *set = calloc(1, sizeof(struct Pset));

	set->capacity = params.initial ? params.initial : PSET_DEFAULT_INITIAL;
	set->vals = calloc(set->capacity, sizeof(void*));

	memcpy((void*)&set->params, &params, sizeof(struct PsetParams));

	return set;
}

const struct Pset *pset_clone(const struct Pset* const from) {
	return from ? clone(from, from->params.alloc_val) : NULL;
}

const struct Pset *pset_clone_deep(const struct Pset* const from) {
	return from && from->params.clone_val ? clone(from, from->params.clone_val) : NULL;
}

void pset_free(const struct Pset * const set) {
	if (!set)
		return;

	free(set->vals);

	free((void*)set);
}

void pset_free_vals(const struct Pset* const set) {
	if (!set)
		return;

	free_all(set);

	pset_free(set);
}

void pset_it_free(const struct PsetIt* const it) {
	if (!it)
		return;

	free((void*)it->st);
	free((void*)it);
}

bool pset_contains(const struct Pset* const set, const void* const val) {
	if (!set || !val)
		return false;

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (set->params.equal_val ? set->params.equal_val(*v, val) : *v == val) {
			return true;
		}
	}

	return false;
}

const void *pset_at(const struct Pset* const set, const size_t i) {
	return set && i < set->size ? *(set->vals + i) : NULL;
}

const void *pset_find(const struct Pset* const set, fn_2pred pred_val, const void* const data) {
	if (!set || !pred_val)
		return NULL;

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (pred_val(*v, data)) {
			return *v;
		}
	}

	return NULL;
}

const struct PsetIt *pset_it(const struct Pset* const set) {
	if (!set || set->size == 0)
		return NULL;

	struct PsetIt *it = calloc(1, sizeof(struct PsetIt));
	it->st = calloc(1, sizeof(struct PsetItState));
	it->st->set = set;

	return pset_it_next(it);
}

const struct PsetIt *pset_filter_it(const struct Pset* const set, fn_2pred pred_val, const void* const data) {
	if (!set || !pred_val || set->size == 0)
		return NULL;

	struct PsetIt *it = calloc(1, sizeof(struct PsetIt));
	it->st = calloc(1, sizeof(struct PsetItState));
	it->st->set = set;
	it->st->pred_val = pred_val;
	it->st->data = data;

	return pset_it_next(it);
}

const struct PsetIt *pset_it_next(const struct PsetIt* const it) {
	if (!it)
		return NULL;

	struct PsetItState *st = it->st;
	if (!st) {
		pset_it_free(it);
		return NULL;
	}

	if (st->attached) {
		st->position++;
	} else {
		st->position = 0;
	}
	st->attached = true;

	for ( ; st->position < st->set->size; st->position++) {

		struct PsetIt *it_m = (struct PsetIt*)it;
		it_m->val = *(st->set->vals + st->position);

		if ((st->pred_val && !st->pred_val(it->val, st->data))) {
			continue;
		}

		return it;
	}

	pset_it_free(it);
	return NULL;
}

bool pset_add(const struct Pset* const set, const void* const val) {
	return set ? add(set, val, set->params.alloc_val) : false;
}

size_t pset_add_all(const struct Pset* const set, const struct Pset* const from) {
	return set && from ? add_all(set, from, set->params.alloc_val) : 0;
}

size_t pset_add_all_clone(const struct Pset* const set, const struct Pset* const from) {
	return set && from && set->params.clone_val ? add_all(set, from, set->params.clone_val) : 0;
}

bool pset_remove(const struct Pset* const set, const void* const val) {
	return set ? remove(set, val, NULL) : false;
}

bool pset_remove_free(const struct Pset* const set, const void* const val) {
	return set ? remove(set, val, set->params.free_val ? set->params.free_val : free) : false;
}

size_t pset_remove_all(const struct Pset* const set) {
	if (!set)
		return 0;

	size_t removed = set->size;

	wipe_vals(set);

	return removed;
}

size_t pset_remove_all_free(const struct Pset* const set) {
	if (!set)
		return 0;

	free_all(set);

	return pset_remove_all(set);
}

size_t pset_remove_from(const struct Pset* const set, const struct Pset* const from) {
	return set && from ? remove_from(set, from, NULL) : 0;
}

size_t pset_remove_from_free(const struct Pset* const set, const struct Pset* const from) {
	return set && from ? remove_from(set, from, set->params.free_val ? set->params.free_val : free) : 0;
}

void pset_it_remove(const struct PsetIt* const it) {
	it_remove(it, false);
}

void pset_it_remove_free(const struct PsetIt* const it) {
	it_remove(it, true);
}

void pset_sort(const struct Pset* const set, fn_less_than less_than_val) {
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

bool pset_equal(const struct Pset* const a, const struct Pset* const b) {
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

struct Pslist *pset_pslist(const struct Pset* const set) {
	return set ? slist(set, set->params.alloc_val) : NULL;
}

struct Pslist *pset_pslist_clone(const struct Pset* const set) {
	if (!set || !set->params.clone_val)
		return NULL;

	return slist(set, set->params.clone_val);
}

char *pset_str(const struct Pset* const set) {
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
size_t pset_size(const struct Pset* const set) {
	return set ? set->size : 0;
}
