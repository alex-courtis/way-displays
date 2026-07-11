#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "fn.h"
#include "pset.h"
#include "pslist.h"
#include "str.h"

#include "ppmap.h"

#define PPMAP_DEFAULT_INITIAL 10
#define PPMAP_DEFAULT_GROW 10

struct PPmap {
	const struct PPmapParams params;
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct PPmapItState {
	const struct PPmap *map;
	size_t position;
	fn_3pred match_key_val;
	fn_2pred match_key;
	fn_2pred match_val;
	const void *data;
};

// grow to capacity + grow
static void grow(struct PPmap *map) {
	size_t new_capacity = map->capacity + (map->params.grow ? map->params.grow : PPMAP_DEFAULT_GROW);

	// grow new arrays
	const void **new_keys = calloc(new_capacity, sizeof(void*));
	const void **new_vals = calloc(new_capacity, sizeof(void*));

	// copy old arrays
	memcpy(new_keys, map->keys, map->capacity * sizeof(void*));
	memcpy(new_vals, map->vals, map->capacity * sizeof(void*));

	// free old arrays
	free(map->keys);
	free(map->vals);

	// lock in new
	map->keys = new_keys;
	map->vals = new_vals;
	map->capacity = new_capacity;
}

static const struct PPmapIt *it_init(const struct PPmap *map) {
	if (map->size == 0)
		return NULL;

	struct PPmapIt *it = calloc(1, sizeof(struct PPmapIt));
	it->st = calloc(1, sizeof(struct PPmapItState));
	it->st->map = map;

	return it;
}

static const void *put(const struct PPmap* const map, const void* const key, const void* const val, fn_clone clone_val) {
	if (!val && !map->params.allow_null_val)
		return NULL;

	const void **k;
	const void **v;
	for (k = map->keys, v = map->vals; k < map->keys + map->size; k++, v++) {

		// overwrite existing values
		if (map->params.equal_key ? map->params.equal_key(*k, key) : *k == key) {
			const void *val_old = *v;

			const void *val_new = val && clone_val ? clone_val(val) : val;
			if (!val_new && !map->params.allow_null_val) {
				return NULL;
			}

			*v = val_new;
			return val_old;
		}
	}

	// alloc new key, never null
	const void *key_new = map->params.alloc_key ? map->params.alloc_key(key) : key;
	if (!key_new)
		return NULL;

	// alloc new val, maybe null
	const void *val_new = val && clone_val ? clone_val(val) : val;
	if (!val_new && !map->params.allow_null_val) {
		return NULL;
	}

	struct PPmap *map_m = (struct PPmap*)map;

	// grow for new entry
	if (map->size >= map->capacity) {
		grow(map_m);
		k = &map->keys[map->size];
		v = &map->vals[map->size];
	}

	// new
	*k = key_new;
	*v = val_new;

	map_m->size++;

	return NULL;
}

static bool put_free(const struct PPmap* const map, const void* const key, const void* const val, fn_clone clone_val) {
	const void *val_old = put(map, key, val, clone_val);

	if (val_old) {
		map->params.free_val ? map->params.free_val(val_old) : free((void*)val_old);
		return true;
	} else {
		return false;
	}
}

static size_t put_all(const struct PPmap* const map, const struct PPmap* const from, fn_clone clone_val, bool do_free) {
	size_t overwritten = 0;

	const void **k;
	const void **v;
	for (k = from->keys, v = from->vals; k < from->keys + from->size; k++, v++) {
		if (do_free) {
			if (put_free(map, *k, *v, clone_val)) {
				overwritten++;
			}
		} else {
			if (put(map, *k, *v, clone_val) != NULL) {
				overwritten++;
			}
		}
	}

	return overwritten;
}

static const struct PPmap *clone(const struct PPmap* const from, fn_clone clone_val) {
	const struct PPmap *to =  ppmap_init_with(from->params);

	const void **k;
	const void **v;
	for (k = from->keys, v = from->vals; k < from->keys + from->size; k++, v++) {
		put(to, *k, *v, clone_val);
	}

	return to;
}

static const struct Pset *vals_pset(const struct PPmap* const map, fn_clone clone_val) {
	const struct PsetParams params = {
		.equal_val = map->params.equal_val,
		.alloc_val = map->params.alloc_val,
		.free_val = map->params.free_val,
		.clone_val = map->params.clone_val,
		.str_val = map->params.str_val,
		.initial = MAX(map->size, map->params.initial),
		.grow  = map->params.grow,
	};
	const struct Pset *set = pset_init_with(params);

	const void **v;
	for (v = map->vals; v < map->vals + map->size; v++) {
		pset_add(set, clone_val && !map->params.alloc_val ? clone_val(*v) : *v);
	}

	return set;
}

static struct Pslist *vals_pslist(const struct PPmap* const map, fn_clone clone_val) {
	struct Pslist *list = NULL;

	const void **k;
	const void **v;
	for (k = map->keys, v = map->vals; k < map->keys + map->size; k++, v++) {
		if (*v && clone_val) {
			pslist_append(&list, (void*)clone_val(*v));
		} else {
			pslist_append(&list, (void*)*v);
		}
	}

	return list;
}

const struct PPmap *ppmap_init(void) {
	const struct PPmapParams params = { 0 };
	return ppmap_init_with(params);
}

const struct PPmap *ppmap_init_with(const struct PPmapParams params) {
	struct PPmap *map = calloc(1, sizeof(struct PPmap));

	map->capacity = params.initial ? params.initial : PPMAP_DEFAULT_INITIAL;
	map->keys = calloc(map->capacity, sizeof(void*));
	map->vals = calloc(map->capacity, sizeof(void*));

	memcpy((void*)&map->params, &params, sizeof(struct PPmapParams));

	return map;
}

const struct PPmap *ppmap_clone(const struct PPmap* const from) {
	return from ? clone(from, from->params.alloc_val) : NULL;
}

const struct PPmap *ppmap_clone_deep(const struct PPmap* const from) {
	return from && from->params.clone_val ? clone(from, from->params.alloc_val ? from->params.alloc_val : from->params.clone_val) : NULL;
}

void ppmap_free(const struct PPmap* const map) {
	if (!map)
		return;

	if (map->params.free_key) {
		for (const void **k = map->keys; k < map->keys + map->size; k++) {
			map->params.free_key(*k);
		}
	}

	free(map->keys);
	free(map->vals);

	free((void*)map);
}

void ppmap_free_vals(const struct PPmap* const map) {
	if (!map)
		return;

	for (const void **v = map->vals; v < map->vals + map->size; v++) {
		if (*v) {
			map->params.free_val ? map->params.free_val (*v) : free((void*)*v);
		}
	}

	ppmap_free(map);
}

void ppmap_it_free(const struct PPmapIt* const it) {
	if (!it)
		return;

	free(it->st);
	free((void*)it);
}

const void *ppmap_get(const struct PPmap* const map, const void* const key) {
	if (!map || !key)
		return NULL;

	const void **k;
	const void **v;
	for (k = map->keys, v = map->vals; k < map->keys + map->size; k++, v++) {
		if (map->params.equal_key ? map->params.equal_key(*k, key) : *k == key) {
			return *v;
		}
	}

	return NULL;
}

bool ppmap_contains_key(const struct PPmap* const map, const void* const key) {
	if (!map || !key)
		return false;

	const void **k;
	for (k = map->keys; k < map->keys + map->size; k++) {
		if (map->params.equal_key ? map->params.equal_key(*k, key) : *k == key) {
			return true;
		}
	}

	return false;
}

bool ppmap_contains_val(const struct PPmap* const map, const void* const val) {
	if (!map)
		return false;

	const void **v;
	for (v = map->vals; v < map->vals + map->size; v++) {
		if (map->params.equal_val ? map->params.equal_val(*v, val) : *v == val) {
			return true;
		}
	}

	return false;
}

struct PPmapPair ppmap_match(const struct PPmap* const map, fn_3pred match, const void* const data) {
	struct PPmapPair res = { 0 };

	if (!map || !match)
		return res;

	const void **k;
	const void **v;
	for (k = map->keys, v = map->vals; k < map->keys + map->size; k++, v++) {
		if (match(*k, *v, data)) {
			res.key = *k;
			res.val = *v;
			break;
		}
	}

	return res;
}

struct PPmapPair ppmap_match_key(const struct PPmap* const map, fn_2pred match, const void* const data) {
	struct PPmapPair res = { 0 };

	if (!map || !match)
		return res;

	const void **k;
	const void **v;
	for (k = map->keys, v = map->vals; k < map->keys + map->size; k++, v++) {
		if (match(*k, data)) {
			res.key = *k;
			res.val = *v;
			break;
		}
	}

	return res;
}

struct PPmapPair ppmap_match_val(const struct PPmap* const map, fn_2pred match, const void* const data) {
	struct PPmapPair res = { 0 };

	if (!map || !match)
		return res;

	const void **k;
	const void **v;
	for (k = map->keys, v = map->vals; k < map->keys + map->size; k++, v++) {
		if (match(*v, data)) {
			res.key = *k;
			res.val = *v;
			break;
		}
	}

	return res;
}

const struct PPmapIt *ppmap_it(const struct PPmap* const map) {
	if (!map)
		return NULL;

	const struct PPmapIt *it = it_init(map);

	if (!it)
		return NULL;

	return ppmap_it_next(it);
}

const struct PPmapIt *ppmap_match_it(const struct PPmap* const map, fn_3pred match, const void* const data) {
	if (!map || !match)
		return NULL;

	const struct PPmapIt *it = it_init(map);
	if (!it)
		return NULL;

	it->st->match_key_val = match;
	it->st->data = data;

	return ppmap_it_next(it);
}

const struct PPmapIt *ppmap_match_key_it(const struct PPmap* const map, fn_2pred match, const void* const data) {
	if (!map || !match)
		return NULL;

	const struct PPmapIt *it = it_init(map);
	if (!it)
		return NULL;

	it->st->match_key = match;
	it->st->data = data;

	return ppmap_it_next(it);
}

const struct PPmapIt *ppmap_match_val_it(const struct PPmap* const map, fn_2pred match, const void* const data) {
	if (!map || !match)
		return NULL;

	const struct PPmapIt *it = it_init(map);
	if (!it)
		return NULL;

	it->st->match_val = match;
	it->st->data = data;

	return ppmap_it_next(it);
}

const struct PPmapIt *ppmap_it_next(const struct PPmapIt* const it) {
	if (!it)
		return NULL;

	struct PPmapItState *st = it->st;

	if (!it->st) {
		ppmap_it_free(it);
		return NULL;
	}

	// null key indicates first use, start at the beginning
	if (it->key) {
		st->position++;
	}

	for ( ; st->position < st->map->size; st->position++) {

		struct PPmapIt *it_m = (struct PPmapIt*)it;

		it_m->key = *(st->map->keys + st->position);
		it_m->val = *(st->map->vals + st->position);

		if (st->match_key_val && !st->match_key_val(it->key, it->val, st->data)) {
			continue;
		}
		if (st->match_key && !st->match_key(it->key, st->data)) {
			continue;
		}
		if (st->match_val && !st->match_val(it->val, st->data)) {
			continue;
		}

		return it;
	}

	ppmap_it_free(it);
	return NULL;
}


const void *ppmap_put(const struct PPmap* const map, const void* const key, const void* const val) {
	return map ? put(map, key, val, map->params.alloc_val) : NULL;
}

const void *ppmap_put_if_absent(const struct PPmap* const map, const void* const key, const void* const val) {
	if (!map || !key)
		return NULL;

	if (ppmap_contains_key(map, key)) {
		return ppmap_get(map, key);
	} else {
		put(map, key, val, map->params.alloc_val);
		return NULL;
	}
}

bool ppmap_put_free(const struct PPmap* const map, const void* const key, const void* const val) {
	return map ? put_free(map, key, val, map->params.alloc_val) : false;
}

size_t ppmap_put_all(const struct PPmap* const map, const struct PPmap* const from) {
	return map && from ? put_all(map, from, map->params.alloc_val, false) : 0;
}

size_t ppmap_put_all_free(const struct PPmap* const map, const struct PPmap* const from) {
	return map && from ? put_all(map, from, map->params.alloc_val, true) : 0;
}

size_t ppmap_put_all_clone(const struct PPmap* const map, const struct PPmap* const from) {
	return map && from && map->params.clone_val ? put_all(map, from, map->params.clone_val, false) : 0;
}

size_t ppmap_put_all_clone_free(const struct PPmap* const map, const struct PPmap* const from) {
	return map && from && map->params.clone_val ? put_all(map, from, map->params.clone_val, true) : 0;
}

const void *ppmap_remove(const struct PPmap* const map, const void* const key) {
	if (!map || !key)
		return NULL;

	const void **k;
	const void **v;
	for (k = map->keys, v = map->vals; k < map->keys + map->size; k++, v++) {

		if (map->params.equal_key ? map->params.equal_key(*k, key) : *k == key) {
			struct PPmap *map_m = (struct PPmap*)map;

			if (map->params.free_key) {
				map->params.free_key((void*)*k);
			}
			*k = NULL;
			const void* val_old = *v;
			*v = NULL;
			map_m->size--;

			// shift down over removed
			const void **mk;
			const void **mv;
			for (mk = k, mv = v; mk < map->keys + map->size; mk++, mv++) {
				*mk = *(mk + 1);
				*mv = *(mv + 1);
			}
			*mk = NULL;
			*mv = NULL;

			return val_old;
		}
	}

	return NULL;
}

bool ppmap_remove_free(const struct PPmap* const map, const void* const key) {
	if (ppmap_contains_key(map, key)) {
		const void *removed = ppmap_remove(map, key);
		if (removed) {
			map->params.free_val ? map->params.free_val(removed) : free((void*)removed);
		}
		return true;
	} else {
		return false;
	}
}

size_t ppmap_remove_all(const struct PPmap* const map, const struct PPmap* const from) {
	if (!map || !from)
		return 0;

	size_t removed = 0;

	const void **k;
	for (k = from->keys; k < from->keys + from->size; k++) {
		if (ppmap_remove(map, *k) != NULL) {
			removed++;
		}
	}

	return removed;
}

size_t ppmap_remove_all_free(const struct PPmap* const map, const struct PPmap* const from) {
	if (!map || !from)
		return 0;

	size_t removed = 0;

	const void **k;
	for (k = from->keys; k < from->keys + from->size; k++) {
		if (ppmap_remove_free(map, *k)) {
			removed++;
		}
	}

	return removed;
}

bool ppmap_equal(const struct PPmap* const a, const struct PPmap* const b) {
	if (!a || !b || a->size != b->size)
		return false;

	const void **ak, **bk;
	const void **av, **bv;

	for (ak = a->keys, bk = b->keys, av = a->vals, bv = b->vals; ak < a->keys + a->size; ak++, bk++, av++, bv++) {

		// key
		if (!(a->params.equal_key ? a->params.equal_key(*ak, *bk) : *ak == *bk)) {
			return false;
		}

		// value
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

struct Pslist *ppmap_keys_pslist(const struct PPmap* const map) {
	if (!map)
		return NULL;

	struct Pslist *list = NULL;

	const void **k;
	for (k = map->keys; k < map->keys + map->size; k++) {
		const void *key = map->params.alloc_key ? map->params.alloc_key(*k) : *k;
		pslist_append(&list, (void*)key);
	}

	return list;
}

const struct Pset *ppmap_keys_pset(const struct PPmap* const map) {
	if (!map)
		return NULL;

	const struct PsetParams params = {
		.equal_val = map->params.equal_key,
		.alloc_val = map->params.alloc_key,
		.free_val = map->params.free_key,
		.clone_val = map->params.alloc_key,
		.str_val = map->params.str_key,
		.initial = MAX(map->size, map->params.initial),
		.grow  = map->params.grow,
	};
	const struct Pset *set = pset_init_with(params);

	const void **k;
	for (k = map->keys; k < map->keys + map->size; k++) {
		pset_add(set, *k);
	}

	return set;
}

struct Pslist *ppmap_vals_pslist(const struct PPmap* const map) {
	return map ? vals_pslist(map, map->params.alloc_val) : NULL;
}

struct Pslist *ppmap_vals_pslist_clone(const struct PPmap* const map) {
	if (!map || !map->params.clone_val)
		return NULL;

	return vals_pslist(map, map->params.clone_val);
}

const struct Pset *ppmap_vals_pset(const struct PPmap* const map) {
	return map ? vals_pset(map, NULL) : NULL;
}

const struct Pset *ppmap_vals_pset_clone(const struct PPmap* const map) {
	return map && map->params.clone_val ? vals_pset(map, map->params.clone_val) : NULL;
}

char *ppmap_str(const struct PPmap* const map) {
	if (!map)
		return NULL;

	char *out = strdup("");

	const void **k;
	const void **v;
	for (k = map->keys, v = map->vals; k < map->keys + map->size; k++, v++) {

		if (*k) {
			if (map->params.str_key) {
				char *key_old = map->params.str_key(*k);
				out = sprintf_append(out, "%s = ", key_old);
				free(key_old);
			} else {
				out = sprintf_append(out, "%p = ", *k);
			}
		} else {
			out = sprintf_append(out, "(null) = ");
		}

		if (*v) {
			if (map->params.str_val) {
				char *val_old = map->params.str_val(*v);
				out = sprintf_append(out, "%s\n", val_old);
				free(val_old);
			} else {
				out = sprintf_append(out, "%p\n", *v);
			}
		} else {
			out = sprintf_append(out, "%s", "(null)\n");
		}
	}

	return out;
}

size_t ppmap_size(const struct PPmap* const map) {
	return map ? map->size : 0;
}
