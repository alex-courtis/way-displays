#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "fn.h"
#include "plist.h"
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
	const struct PsetFilter filter;
	size_t position;
	bool attached;
};

// grow to capacity + grow
static void grow(const struct Pset *set) {
	size_t new_capacity = set->capacity + (set->params.grow ? set->params.grow : PSET_DEFAULT_GROW);

	// grow new arrays
	const void **new_vals = calloc(new_capacity, sizeof(void*));

	// copy old arrays
	memcpy(new_vals, set->vals, set->capacity * sizeof(void*));

	// free old arrays
	free(set->vals);

	// lock in new
	((struct Pset*)set)->vals = new_vals;
	((struct Pset*)set)->capacity = new_capacity;
}

static const struct PsetIt *it_init(const struct Pset *set, const struct PsetFilter *filter) {
	if (set->size == 0)
		return NULL;

	struct PsetIt *it = calloc(1, sizeof(struct PsetIt));
	it->st = calloc(1, sizeof(struct PsetItState));
	it->st->set = set;

	if (filter) {
		memcpy((void*)&it->st->filter, filter, sizeof(struct PsetFilter));
	}

	return it;
}

static bool add(const struct Pset* const set, const void* const val, fn_clone alloc_val) {
	if (!val)
		return false;

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (set->params.equal_val ? set->params.equal_val(*v, val) : *v == val) {
			return false;
		}
	}

	// create new value
	const void *new = alloc_val ? alloc_val(val) : val;
	if (!new)
		return false;

	// maybe grow for new entry
	if (set->size >= set->capacity) {
		grow(set);
	}

	// assign new value
	set->vals[set->size] = new;
	((struct Pset*)set)->size++;

	return true;
}

static bool remove(const void **removed, const struct Pset* const set, const void* const val, bool do_free) {
	if (!val)
		return false;

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (set->params.equal_val ? set->params.equal_val(*v, val) : *v == val) {
			if (removed) {
				*removed = *v;
			}

			if (do_free) {
				if (set->params.free_val) {
					set->params.free_val((void*)*v);
				} else {
					free((void*)*v);
				}
			}

			*v = NULL;
			((struct Pset*)set)->size--;

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

static void free_vals(const struct Pset* const set) {
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

static size_t remove_all(const struct Pset* const set, bool do_free) {
	size_t removed = set->size;
	((struct Pset*)set)->size = 0;

	memset(set->vals, 0, set->size * sizeof(void*));

	return removed;
}

static bool it_remove(const void **removed, const struct PsetIt* const it, bool do_free) {
	struct PsetItState *st = it->st;
	if (!st) {
		pset_it_free(it);
		return false;
	}

	bool was_removed = remove(removed, st->set, it->val, do_free);

	if (st->position > 0) {
		st->position--;
	} else {
		st->attached = false;
	}

	((struct PsetIt*)it)->val = NULL;

	return was_removed;
}

static bool filter_blocks(const struct PsetFilter *filter, const void* const val) {
	return
		(filter->val          && !filter->val         (val              )) ||
		(filter->val_data     && !filter->val_data    (val, filter->data));
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

static size_t remove_in(const struct Pset* const set, const struct Pset* const in, bool do_free) {
	size_t removed = 0;

	for (const void **v = in->vals; v < in->vals + in->size; v++) {
		if (remove(NULL, set, *v, do_free)) {
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

static const struct Plist *plist(const struct Pset* const set, fn_clone clone_val) {
	const struct PlistParams params = {
		.equal_val = set->params.equal_val,
		.alloc_val = set->params.alloc_val,
		.free_val = set->params.free_val,
		.clone_val = set->params.clone_val,
		.str_val = set->params.str_val,
		.initial = MAX(set->size, set->params.initial),
		.grow  = set->params.grow,
	};
	const struct Plist *list = plist_init_with(params);

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (clone_val) {
			plist_append(list, (void*)clone_val(*v));
		} else {
			plist_append(list, (void*)*v);
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

	free_vals(set);

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

const void *pset_find(const struct Pset* const set, const struct PsetFilter filter) {
	if (!set)
		return NULL;

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (!filter_blocks(&filter, *v)) {
			return *v;
		}
	}

	return NULL;
}

const struct PsetIt *pset_it(const struct Pset* const set) {
	return set ? pset_it_next(it_init(set, NULL)) : NULL;
}

const struct PsetIt *pset_filter_it(const struct Pset* const set, const struct PsetFilter filter) {
	return set ? pset_it_next(it_init(set, &filter)) : NULL;
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

		if (filter_blocks(&st->filter, it->val)) {
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

bool pset_add_clone(const struct Pset* const set, const void* const val) {
	return set && set->params.clone_val ? add(set, val, set->params.clone_val) : false;
}

size_t pset_add_all(const struct Pset* const set, const struct Pset* const from) {
	return set && from ? add_all(set, from, set->params.alloc_val) : 0;
}

size_t pset_add_all_clone(const struct Pset* const set, const struct Pset* const from) {
	return set && from && set->params.clone_val ? add_all(set, from, set->params.clone_val) : 0;
}

const void *pset_remove(const struct Pset* const set, const void* const val) {
	if (!set)
		return NULL;

	const void *removed = NULL;
	remove(&removed, set, val, false);

	return removed;
}

bool pset_remove_free(const struct Pset* const set, const void* const val) {
	return set ? remove(NULL, set, val, true) : false;
}

size_t pset_remove_all(const struct Pset* const set) {
	return set ? remove_all(set, false) : 0;
}

size_t pset_remove_all_free(const struct Pset* const set) {
	if (!set)
		return 0;

	free_vals(set);

	return remove_all(set, true);
}

size_t pset_remove_in(const struct Pset* const set, const struct Pset* const in) {
	return set && in ? remove_in(set, in, false) : 0;
}

size_t pset_remove_in_free(const struct Pset* const set, const struct Pset* const in) {
	return set && in ? remove_in(set, in, true) : 0;
}

const void *pset_it_remove(const struct PsetIt* const it) {
	if (!it)
		return NULL;

	const void *removed = NULL;
	it_remove(&removed, it, false);

	return removed;
}

bool pset_it_remove_free(const struct PsetIt* const it) {
	return it ? it_remove(NULL, it, true) : false;
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

	for (const void **bv = b->vals; bv < (b->vals + b->size); bv++) {
		if (!pset_contains(a, *bv)) {
			return false;
		}
	}

	return true;
}

bool pset_equal_ordered(const struct Pset* const a, const struct Pset* const b) {
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

const struct Plist *pset_plist(const struct Pset* const set) {
	return set ? plist(set, NULL) : NULL;
}

const struct Plist *pset_plist_clone(const struct Pset* const set) {
	if (!set || !set->params.clone_val)
		return NULL;

	return plist(set, set->params.clone_val);
}

char *pset_str(const struct Pset* const set) {
	if (!set)
		return NULL;

	char *out = strdup("");

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (set->params.str_val) {
			char *val_str = set->params.str_val(*v);
			if (val_str) {
				out = sprintf_append(out, "%s\n", val_str);
				free(val_str);
			} else {
				out = sprintf_append(out, "(null)\n");
			}
		} else {
			if (*v) {
				out = sprintf_append(out, "%p\n", *v);
			} else {
				out = sprintf_append(out, "(null)\n");
			}
		}
	}

	return out;
}
size_t pset_size(const struct Pset* const set) {
	return set ? set->size : 0;
}
