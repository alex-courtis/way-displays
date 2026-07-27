#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "str.h"

#include "plist.h"

#define PLIST_DEFAULT_INITIAL 10
#define PLIST_DEFAULT_GROW 10

struct Plist {
	const struct PlistParams params;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct PlistItState {
	const struct Plist *list;
	const struct PlistFilter filter;
	size_t position;
	bool attached;
	bool was_next; // last iteration was a next, otherwise prev
};

// grow to capacity + grow
static void grow(struct Plist *list) {
	size_t new_capacity = list->capacity + (list->params.grow ? list->params.grow : PLIST_DEFAULT_GROW);

	// grow new arrays
	const void **new_vals = calloc(new_capacity, sizeof(void*));

	// copy old arrays
	memcpy(new_vals, list->vals, list->capacity * sizeof(void*));

	// free old arrays
	free(list->vals);

	// lock in new
	list->vals = new_vals;
	list->capacity = new_capacity;
}

static const struct PlistIt *it_init(const struct Plist *list, const struct PlistFilter *filter) {
	if (list->size == 0)
		return NULL;

	struct PlistIt *it = calloc(1, sizeof(struct PlistIt));
	it->st = calloc(1, sizeof(struct PlistItState));
	it->st->list = list;

	if (filter) {
		memcpy((void*)&it->st->filter, filter, sizeof(struct PlistFilter));
	}

	return it;
}

static bool insert(const struct Plist* const list, size_t index, const void* const val, fn_clone alloc_val) {
	if (!val && !list->params.allow_null_val) {
		return false;
	}

	if (index > list->size) {
		index = list->size;
	}

	// create new value; alloc_val may return a valid new from a NULL val
	const void *new = alloc_val ? alloc_val(val) : val;

	if (!new && !list->params.allow_null_val) {
		return false;
	}

	// maybe grow for new entry
	if (list->size >= list->capacity) {
		grow((struct Plist*)list);
	}

	// shift later down
	if (index < list->size) {
		for (size_t i = list->size; i > index; i--) {
			list->vals[i] = list->vals[i - 1];
		}
	}

	((struct Plist*)list)->size++;

	list->vals[index] = new;

	return true;
}

static bool replace(const void **replaced, const struct Plist* const list, size_t index, const void* const val, bool do_free) {
	if (replaced)
		*replaced = NULL;

	if (index >= list->size)
		return false;

	// create new value; alloc_val may return a valid new from a NULL val
	const void *new = list->params.alloc_val ? list->params.alloc_val(val) : val;

	if (!new && !list->params.allow_null_val) {
		return false;
	}

	const void *old = list->vals[index];

	if (replaced) {
		*replaced = old;
	}

	if (do_free && old) {
		if (list->params.free_val) {
			list->params.free_val((void*)old);
		} else {
			free((void*)old);
		}
	}

	list->vals[index] = new;

	return true;
}

static bool remove_at(const void **removed, const struct Plist* const list, const size_t i, bool do_free) {
	if (removed)
		*removed = NULL;

	if (i >= list->size)
		return false;

	const void **v = list->vals + i;
	const void *old = *v;

	if (removed) {
		*removed = old;
	}

	if (do_free && old) {
		if (list->params.free_val) {
			list->params.free_val((void*)old);
		} else {
			free((void*)old);
		}
	}

	*v = NULL;
	((struct Plist*)list)->size--;

	// shift down over removed
	const void **m;
	for (m = v; m < list->vals + list->size; m++) {
		*m = *(m + 1);
	}
	*m = NULL;

	return true;
}

static size_t remove_in(const struct Plist* const list, const struct Plist* const in, bool do_free) {
	size_t removed = 0;

	// values freed
	const void **freed = calloc(list->size, sizeof(void*));
	size_t nf = 0;

	for (const void **v = in->vals; v < in->vals + in->size; v++) {
		size_t i = 0;
		while (plist_index_of(&i, list, *v)) {
			removed++;

			const void *val = NULL;
			remove_at(&val, list, i, false);

			if (val && do_free) {
				bool already_freed = false;

				for (const void **vf = freed; vf < freed + nf; vf++) {
					if (val == *vf) {
						already_freed = true;
						break;
					}
				}

				if (!already_freed) {
					list->params.free_val ? list->params.free_val((void*)val) : free((void*)val);
					*(freed + nf++) = val;
				}
			}
		}
	}

	free(freed);

	return removed;
}

static void free_vals(const struct Plist* const list) {

	// values freed
	const void **freed = calloc(list->size, sizeof(void*));
	size_t nf = 0;

	for (const void **vl = list->vals; vl < list->vals + list->size; vl++) {
		if (!*vl)
			continue;

		bool already_freed = false;

		for (const void **vf = freed; vf < freed + nf; vf++) {
			if (*vl == *vf) {
				already_freed = true;
				break;
			}
		}

		if (!already_freed) {
			list->params.free_val ? list->params.free_val((void*)*vl) : free((void*)*vl);
			*(freed + nf++) = *vl;
		}
	}

	free(freed);
}

static size_t remove_all(const struct Plist* const list) {
	memset(list->vals, 0, list->size * sizeof(void*));

	size_t removed = list->size;

	((struct Plist*)list)->size = 0;

	return removed;
}

static bool it_remove(const void **removed, const struct PlistIt* const it, bool do_free) {
	if (removed)
		*removed = NULL;

	if (!it)
		return false;

	struct PlistItState *st = it->st;
	if (!st) {
		plist_it_free(it);
		return false;
	}

	bool was_removed = remove_at(removed, st->list, st->position, do_free);

	((struct PlistIt*)it)->val = NULL;

	if (st->was_next) {
		if (st->position > 0) {
			st->position--;
		} else {
			st->attached = false;
		}
	}

	return was_removed;
}

static bool filter_blocks(const struct PlistFilter *filter, const void* const val) {
	return
		(filter->val          && !filter->val         (val              )) ||
		(filter->val_data     && !filter->val_data    (val, filter->data));
}

static size_t append_all(const struct Plist* const list, const struct Plist* const from, fn_clone clone_val) {
	size_t appended = 0;

	for (const void **v = from->vals; v < from->vals + from->size; v++) {
		if (insert(list, list->size, *v, clone_val)) {
			appended++;
		}
	}

	return appended;
}

static const struct Plist *clone(const struct Plist* const from, fn_clone clone_val) {
	const struct Plist *to = plist_init_with(from->params);

	for (const void **v = from->vals; v < from->vals + from->size; v++) {
		insert(to, to->size, *v, clone_val);
	}

	return to;
}

const struct Plist *plist_init(void) {
	const struct PlistParams params = { 0 };
	return plist_init_with(params);
}

const struct Plist *plist_init_with(const struct PlistParams params) {
	struct Plist *list = calloc(1, sizeof(struct Plist));

	list->capacity = params.initial ? params.initial : PLIST_DEFAULT_INITIAL;
	list->vals = calloc(list->capacity, sizeof(void*));

	memcpy((void*)&list->params, &params, sizeof(struct PlistParams));

	return list;
}

const struct Plist *plist_clone(const struct Plist* const from) {
	return from ? clone(from, from->params.alloc_val) : NULL;
}

const struct Plist *plist_clone_deep(const struct Plist* const from) {
	return from && from->params.clone_val ? clone(from, from->params.clone_val) : NULL;
}

void plist_free(const struct Plist * const list) {
	if (!list)
		return;

	free(list->vals);

	free((void*)list);
}

void plist_free_vals(const struct Plist* const list) {
	if (!list)
		return;

	free_vals(list);

	plist_free(list);
}

void plist_it_free(const struct PlistIt* const it) {
	if (!it)
		return;

	free((void*)it->st);
	free((void*)it);
}

bool plist_contains(const struct Plist* const list, const void* const val) {
	return plist_index_of(NULL, list, val);
}

bool plist_index_of(size_t *index, const struct Plist* const list, const void* const val) {
	if (index)
		*index = 0;

	if (!list)
		return false;

	if (!val && !list->params.allow_null_val)
		return false;

	for (size_t i = 0; i < list->size; i++) {
		const void **v = list->vals + i;
		if (list->params.equal_val ? list->params.equal_val(*v, val) : *v == val) {
			if (index) {
				*index = i;
			}
			return true;
		}
	}

	return false;
}

const void *plist_at(const struct Plist* const list, const size_t i) {
	return list && i < list->size ? *(list->vals + i) : NULL;
}

const void *plist_find(const struct Plist* const list, const struct PlistFilter filter) {
	if (!list)
		return NULL;

	for (const void **v = list->vals; v < list->vals + list->size; v++) {
		if (!filter_blocks(&filter, *v)) {
			return *v;
		}
	}

	return NULL;
}

const struct PlistIt *plist_it_start(const struct Plist* const list) {
	return list ? plist_it_next(it_init(list, NULL)) : NULL;
}

const struct PlistIt *plist_it_end(const struct Plist* const list) {
	return list ? plist_it_prev(it_init(list, NULL)) : NULL;
}

const struct PlistIt *plist_filter_it_start(const struct Plist* const list, const struct PlistFilter filter) {
	return list ? plist_it_next(it_init(list, &filter)) : NULL;
}

const struct PlistIt *plist_filter_it_end(const struct Plist* const list, const struct PlistFilter filter) {
	return list ? plist_it_prev(it_init(list, &filter)) : NULL;
}

const struct PlistIt *plist_it_next(const struct PlistIt* const it) {
	if (!it)
		return NULL;

	struct PlistItState *st = it->st;
	if (!st) {
		plist_it_free(it);
		return NULL;
	}

	if (st->attached) {
		st->position++;
	} else {
		st->position = 0;
	}
	st->attached = true;

	st->was_next = true;

	for ( ; st->position < st->list->size; st->position++) {

		((struct PlistIt*)it)->val = *(st->list->vals + st->position);

		if (filter_blocks(&st->filter, it->val)) {
			continue;
		}

		return it;
	}

	plist_it_free(it);
	return NULL;
}

const struct PlistIt *plist_it_prev(const struct PlistIt* const it) {
	if (!it)
		return NULL;

	struct PlistItState *st = it->st;
	if (!st) {
		plist_it_free(it);
		return NULL;
	}

	if (st->attached) {
		st->position--;
	} else {
		st->position = st->list->size - 1;
	}
	st->attached = true;

	st->was_next = false;

	for (size_t i = st->position + 1; i > 0; i--) {
		st->position = i - 1;

		((struct PlistIt*)it)->val = *(st->list->vals + st->position);

		if (filter_blocks(&st->filter, it->val)) {
			continue;
		}

		return it;
	}

	plist_it_free(it);
	return NULL;
}

bool plist_insert(const struct Plist* const list, size_t index, const void* const val) {
	return list ? insert(list, index, val, list->params.alloc_val) : false;
}

bool plist_append(const struct Plist* const list, const void* const val) {
	return list ? insert(list, list->size, val, list->params.alloc_val) : false;
}

bool plist_prepend(const struct Plist* const list, const void* const val) {
	return list ? insert(list, 0, val, list->params.alloc_val) : false;
}

const void *plist_replace(const struct Plist* const list, size_t index, const void* const val) {
	if (!list)
		return NULL;

	const void *replaced = NULL;
	replace(&replaced, list, index, val, false);

	return replaced;
}

bool plist_replace_free(const struct Plist* const list, size_t index, const void* const val) {
	return list ? replace(NULL, list, index, val, true) : false;
}

size_t plist_append_all(const struct Plist* const list, const struct Plist* const from) {
	return list && from ? append_all(list, from, list->params.alloc_val) : 0;
}

size_t plist_append_all_clone(const struct Plist* const list, const struct Plist* const from) {
	return list && from && list->params.clone_val ? append_all(list, from, list->params.clone_val) : 0;
}

const void *plist_remove(const struct Plist* const list, const void* const val) {
	if (!list)
		return NULL;

	size_t i;
	if (!plist_index_of(&i, list, val))
		return NULL;

	const void *removed = NULL;
	remove_at(&removed, list, i, false);

	return removed;
}

bool plist_remove_free(const struct Plist* const list, const void* const val) {
	if (!list)
		return NULL;

	size_t i;
	if (!plist_index_of(&i, list, val))
		return false;

	return remove_at(NULL, list, i, true);
}

const void *plist_remove_at(const struct Plist* const list, const size_t i) {
	if (!list)
		return NULL;

	const void *removed = NULL;
	remove_at(&removed, list, i, false);

	return removed;
}

bool plist_remove_at_free(const struct Plist* const list, const size_t i) {
	return list ? remove_at(NULL, list, i, true) : false;
}

size_t plist_remove_all(const struct Plist* const list) {
	return list ? remove_all(list) : 0;
}

size_t plist_remove_all_free(const struct Plist* const list) {
	if (!list)
		return 0;

	free_vals(list);

	return remove_all(list);
}

size_t plist_remove_in(const struct Plist* const map, const struct Plist* const in) {
	return map && in ? remove_in(map, in, false) : 0;
}

size_t plist_remove_in_free(const struct Plist* const map, const struct Plist* const in) {
	return map && in ? remove_in(map, in, true) : 0;
}

const void *plist_it_remove(const struct PlistIt* const it) {
	if (!it)
		return NULL;

	const void *removed = NULL;
	it_remove(&removed, it, false);

	return removed;
}

bool plist_it_remove_free(const struct PlistIt* const it) {
	return it_remove(NULL, it, true);
}

void plist_sort(const struct Plist* const list, fn_less_than less_than_val) {
	if (!list || !less_than_val)
		return;

	static const size_t gaps[] = { 701, 301, 132, 57, 23, 10, 4, 1, 0 }; // Ciura gap sequence

	for (const size_t *gap = gaps; *gap > 0; gap++) {
		for (size_t i = *gap; i < list->size; i++) {
			const void *tmp = list->vals[i];
			size_t j;
			for (j = i; (j >= *gap) && less_than_val(tmp, list->vals[j - *gap]); j -= *gap) {
				list->vals[j] = list->vals[j - *gap];
			}
			list->vals[j] = tmp;
		}
	}
}

bool plist_equal(const struct Plist* const a, const struct Plist* const b) {
	if (!a || !b || a->size != b->size)
		return false;

	for (const void **bv = b->vals; bv < (b->vals + b->size); bv++) {
		if (!plist_contains(a, *bv)) {
			return false;
		}
	}

	return true;
}

bool plist_equal_ordered(const struct Plist* const a, const struct Plist* const b) {
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

char *plist_str(const struct Plist* const list) {
	if (!list)
		return NULL;

	char *out = strdup("");

	for (const void **v = list->vals; v < list->vals + list->size; v++) {
		if (list->params.str_val) {
			char *val_str = list->params.str_val(*v);
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
size_t plist_size(const struct Plist* const list) {
	return list ? list->size : 0;
}
