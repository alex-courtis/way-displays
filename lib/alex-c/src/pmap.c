#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"
#include "str.h"

#include "pmap.h"

#define PMAP_DEFAULT_INITIAL 10
#define PMAP_DEFAULT_GROW 10

struct PMap {
	const struct PMapParams params;
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct PMapItState {
	const struct PMap *map;
	size_t position;
	fn_match_key_val match;
	const void *data;
};

// grow to capacity + grow
static void grow(struct PMap *map) {
	size_t new_capacity = map->capacity + (map->params.grow ? map->params.grow : PMAP_DEFAULT_GROW);

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

static const void *put(const struct PMap* const cmap, const void* const key, const void* const val, fn_alloc alloc_val) {
	if (!key)
		return NULL;

	struct PMap *map = (struct PMap*)cmap;

	const void **k;
	const void **v;
	for (k = map->keys, v = map->vals; k < map->keys + map->size; k++, v++) {

		// overwrite existing values
		if (map->params.equal_key ? map->params.equal_key(*k, key) : *k == key) {
			const void *val_old = *v;
			if (val && alloc_val) {
				*v = alloc_val(val);
			} else {
				*v = val;
			}
			return val_old;
		}
	}

	// create new key
	const void *key_new = map->params.alloc_key ? map->params.alloc_key(key) : key;
	if (!key_new)
		return NULL;

	// grow for new entry
	if (map->size >= map->capacity) {
		grow(map);
		k = &map->keys[map->size];
		v = &map->vals[map->size];
	}

	// new
	*k = key_new;
	if (val && alloc_val) {
		*v = alloc_val(val);
	} else {
		*v = val;
	}

	map->size++;

	return NULL;
}

static const struct PMap *clone(const struct PMap* const from, fn_clone clone_val) {
	const struct PMap *to =  pmap_init_with(from->params);

	const void **k;
	const void **v;
	for (k = from->keys, v = from->vals; k < from->keys + from->size; k++, v++) {
		put(to, *k, *v, clone_val);
	}

	return to;
}

static struct SList *keys_slist(const struct PMap* const map, fn_clone clone_key) {
	struct SList *list = NULL;

	const void **k;
	for (k = map->keys; k < map->keys + map->size; k++) {
		if (clone_key) {
			slist_append(&list, (void*)clone_key(*k));
		} else {
			slist_append(&list, (void*)*k);
		}
	}

	return list;
}

static struct SList *vals_slist(const struct PMap* const map, fn_clone clone_val) {
	struct SList *list = NULL;

	const void **k;
	const void **v;
	for (k = map->keys, v = map->vals; k < map->keys + map->size; k++, v++) {
		if (*v && clone_val) {
			slist_append(&list, (void*)clone_val(*v));
		} else {
			slist_append(&list, (void*)*v);
		}
	}

	return list;
}

const struct PMap *pmap_init(void) {
	const struct PMapParams params = { 0 };
	return pmap_init_with(params);
}

const struct PMap *pmap_init_with(const struct PMapParams params) {
	struct PMap *map = calloc(1, sizeof(struct PMap));

	map->capacity = params.initial ? params.initial : PMAP_DEFAULT_INITIAL;
	map->keys = calloc(map->capacity, sizeof(void*));
	map->vals = calloc(map->capacity, sizeof(void*));

	memcpy((void*)&map->params, &params, sizeof(struct PMapParams));

	return map;
}

const struct PMap *pmap_clone_shallow(const struct PMap* const from) {
	return from ? clone(from, NULL) : NULL;
}

const struct PMap *pmap_clone_deep(const struct PMap* const from) {
	if (!from)
		return NULL;

	if (from->params.clone_val)
		return clone(from, from->params.clone_val);
	else
		return pmap_init_with(from->params);
}

void pmap_free(const struct PMap* const map) {
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

void pmap_free_vals(const struct PMap* const map) {
	if (!map)
		return;

	for (const void **v = map->vals; v < map->vals + map->size; v++) {
		if (*v) {
			if (map->params.free_val) {
				map->params.free_val(*v);
			} else {
				free((void*)*v);
			}
		}
	}

	pmap_free(map);
}

void pmap_it_free(const struct PMapIt* const it) {
	if (!it)
		return;

	free(it->st);
	free((void*)it);
}

const void *pmap_get(const struct PMap* const map, const void* const key) {
	if (!map || !key)
		return NULL;

	const void **k;
	const void **v;
	for (k = map->keys, v = map->vals;
			k < map->keys + map->size;
			k++, v++) {
		if (map->params.equal_key ? map->params.equal_key(*k, key) : *k == key) {
			return *v;
		}
	}

	return NULL;
}

bool pmap_contains_key(const struct PMap* const map, const void* const key) {
	if (!map || !key)
		return false;

	const void **k;
	for (k = map->keys;
			k < map->keys + map->size;
			k++) {
		if (map->params.equal_key ? map->params.equal_key(*k, key) : *k == key) {
			return true;
		}
	}

	return false;
}

struct PMapPair pmap_match(const struct PMap* const map, fn_match_key_val match, const void* const data) {
	struct PMapPair res = { 0 };

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

const struct PMapIt *pmap_it(const struct PMap* const map) {
	if (!map || map->size == 0)
		return NULL;

	struct PMapIt *it = calloc(1, sizeof(struct PMapIt));
	it->st = calloc(1, sizeof(struct PMapItState));
	it->st->map = map;

	return pmap_it_next(it);
}

const struct PMapIt *pmap_match_it(const struct PMap* const map, fn_match_key_val match, const void* const data) {
	if (!map || !match || map->size == 0)
		return NULL;

	struct PMapIt *it = calloc(1, sizeof(struct PMapIt));
	it->st = calloc(1, sizeof(struct PMapItState));
	it->st->map = map;
	it->st->match = match;
	it->st->data = data;

	return pmap_it_next(it);
}

const struct PMapIt *pmap_it_next(const struct PMapIt* const cit) {
	if (!cit)
		return NULL;

	struct PMapIt *it = (struct PMapIt*)cit;
	struct PMapItState *st = it->st;

	if (!it->st) {
		pmap_it_free(it);
		return NULL;
	}

	// null key indicates first use, start at the beginning
	if (it->key) {
		st->position++;
	}

	for ( ; st->position < st->map->size; st->position++) {

		it->key = *(st->map->keys + st->position);
		it->val = *(st->map->vals + st->position);

		if (st->match && !st->match(it->key, it->val, st->data)) {
			continue;
		}

		return it;
	}

	pmap_it_free(it);
	return NULL;
}


const void *pmap_put(const struct PMap* const map, const void* const key, const void* const val) {
	return map ? put(map, key, val, map->params.alloc_val) : NULL;
}

const void *pmap_put_if_absent(const struct PMap* const map, const void* const key, const void* const val) {
	if (!map || !key)
		return NULL;

	if (pmap_contains_key(map, key)) {
		return pmap_get(map, key);
	} else {
		put(map, key, val, map->params.alloc_val);
		return NULL;
	}
}

bool pmap_put_free(const struct PMap* const map, const void* const key, const void* const val) {
	if (!map)
		return false;

	const void *val_old = put(map, key, val, map->params.alloc_val);

	if (val_old) {
		if (map->params.free_val) {
			map->params.free_val(val_old);
		} else {
			free((void*)val_old);
		}
		return true;
	} else {
		return false;
	}
}

const void *pmap_remove(const struct PMap* const cmap, const void* const key) {
	if (!cmap || !key)
		return NULL;

	struct PMap *map = (struct PMap*)cmap;

	const void **k;
	const void **v;
	for (k = map->keys, v = map->vals; k < map->keys + map->size; k++, v++) {

		if (map->params.equal_key ? map->params.equal_key(*k, key) : *k == key) {
			if (map->params.free_key) {
				map->params.free_key((void*)*k);
			}
			*k = NULL;
			const void* val_old = *v;
			*v = NULL;
			map->size--;

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

bool pmap_remove_free(const struct PMap* const map, const void* const key) {
	if (pmap_contains_key(map, key)) {
		const void *removed = pmap_remove(map, key);
		if (removed) {
			if (map->params.free_val) {
				map->params.free_val(removed);
			} else {
				free((void*)removed);
			}
		}
		return true;
	} else {
		return false;
	}
}

bool pmap_equal(const struct PMap* const a, const struct PMap* const b) {
	if (!a || !b || a->size != b->size)
		return false;

	const void **ak, **bk;
	const void **av, **bv;

	for (ak = a->keys, bk = b->keys, av = a->vals, bv = b->vals;
			ak < a->keys + a->size;
			ak++, bk++, av++, bv++) {

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

struct SList *pmap_keys_slist_shallow(const struct PMap* const map) {
	return map ? keys_slist(map, NULL) : NULL;
}

struct SList *pmap_keys_slist_deep(const struct PMap* const map) {
	if (!map || !map->params.alloc_key)
		return NULL;

	return keys_slist(map, map->params.alloc_key);
}

struct SList *pmap_vals_slist_shallow(const struct PMap* const map) {
	return map ? vals_slist(map, NULL) : NULL;
}

struct SList *pmap_vals_slist_deep(const struct PMap* const map) {
	if (!map || !map->params.clone_val)
		return NULL;

	return vals_slist(map, map->params.clone_val);
}

char *pmap_str(const struct PMap* const map) {
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

size_t pmap_size(const struct PMap* const map) {
	return map ? map->size : 0;
}
