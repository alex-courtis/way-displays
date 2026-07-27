#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "fn.h"
#include "pset.h"
#include "plist.h"
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
	const struct PPmapFilter filter;
	size_t position;
	bool attached;
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

static const struct PPmapIt *it_init(const struct PPmap *map, const struct PPmapFilter *filter) {
	if (map->size == 0)
		return NULL;

	struct PPmapIt *it = calloc(1, sizeof(struct PPmapIt));
	it->st = calloc(1, sizeof(struct PPmapItState));
	it->st->map = map;

	if (filter) {
		memcpy((void*)&it->st->filter, filter, sizeof(struct PPmapFilter));
	}

	return it;
}

static bool filter_blocks(const struct PPmapFilter *filter, const void* const key, const void* const val) {
	return
		(filter->key          && !filter->key         (key                    )) ||
		(filter->val          && !filter->val         (      val              )) ||
		(filter->key_val      && !filter->key_val     (key,  val              )) ||
		(filter->key_data     && !filter->key_data    (key,       filter->data)) ||
		(filter->val_data     && !filter->val_data    (      val, filter->data)) ||
		(filter->key_val_data && !filter->key_val_data(key,  val, filter->data));
}

// return true if overwritten - val_old populated, maybe NULL
static bool put(const void **val_old, const struct PPmap* const map, const void* const key, const void* const val, fn_clone alloc_val, bool overwrite) {
	if (!val && !map->params.allow_null_val)
		return NULL;

	const void **k;
	const void **v;
	for (k = map->keys, v = map->vals; k < map->keys + map->size; k++, v++) {

		// overwrite existing values
		if (map->params.equal_key ? map->params.equal_key(*k, key) : *k == key) {
			*val_old = *v;

			if (!overwrite) {
				return true;
			}

			const void *val_new = alloc_val ? alloc_val(val) : val;
			if (!val_new && !map->params.allow_null_val) {
				return true;
			}

			*v = val_new;
			return true;
		}
	}

	// alloc new key, never null
	const void *key_new = map->params.alloc_key ? map->params.alloc_key(key) : key;
	if (!key_new)
		return false;

	// alloc new value; alloc_val may return a valid new from a NULL val
	const void *val_new = alloc_val ? alloc_val(val) : val;
	if (!val_new && !map->params.allow_null_val) {
		return false;
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

	return false;
}

static size_t put_all(const struct PPmap* const map, const struct PPmap* const from, fn_clone clone_val, bool do_free) {
	size_t overwritten = 0;

	// values already freed
	const void **freed = calloc(map->size, sizeof(void*));
	size_t nf = 0;

	for (const void **k = from->keys, **v = from->vals; k < from->keys + from->size; k++, v++) {
		const void *val_old = NULL;

		if (put(&val_old, map, *k, *v, clone_val, true)) {
			overwritten++;

			if (do_free && val_old) {
				bool already_freed = false;
				for (const void **vf = freed; vf < freed + nf; vf++) {
					if (val_old == *vf) {
						already_freed = true;
						break;
					}
				}

				if (!already_freed) {
					map->params.free_val ? map->params.free_val((void*)val_old) : free((void*)val_old);
					*(freed + nf++) = val_old;
				}
			}
		}
	}

	free(freed);

	return overwritten;
}

static const struct PPmap *clone(const struct PPmap* const from, fn_clone clone_val) {
	const struct PPmap *to =  ppmap_init_with(from->params);

	const void *val_old = NULL;
	for (const void **k = from->keys, **v = from->vals; k < from->keys + from->size; k++, v++) {
		put(&val_old, to, *k, *v, clone_val, true);
	}

	return to;
}

static void free_keys(const struct PPmap* const map) {
	if (!map->params.free_key)
		return;

	for (const void **k = map->keys; k < map->keys + map->size; k++) {
		if (*k) {
			map->params.free_key((void*)*k);
		}
	}
}

static void free_vals(const struct PPmap* const map) {

	// values already freed
	const void **freed = calloc(map->size, sizeof(void*));
	size_t nf = 0;

	for (const void **v = map->vals; v < map->vals + map->size; v++) {
		if (!*v)
			continue;

		bool already_freed = false;
		for (const void **vf = freed; vf < freed + nf; vf++) {
			if (*v == *vf) {
				already_freed = true;
				break;
			}
		}

		if (!already_freed) {
			map->params.free_val ? map->params.free_val((void*)*v) : free((void*)*v);
			*(freed + nf++) = *v;
		}
	}

	free(freed);
}

static bool remove_(const void **removed, const struct PPmap* const map, const void* const key, bool do_free) {
	if (removed)
		*removed = NULL;

	if (!key)
		return NULL;

	bool was_removed = false;

	const void *val_old = NULL;

	for (const void **k = map->keys, **v = map->vals; k < map->keys + map->size; k++, v++) {

		if (map->params.equal_key ? map->params.equal_key(*k, key) : *k == key) {
			struct PPmap *map_m = (struct PPmap*)map;

			if (map->params.free_key) {
				map->params.free_key((void*)*k);
			}
			*k = NULL;
			val_old = *v;
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

			was_removed = true;
			break;
		}
	}

	if (removed) {
		*removed = val_old;
	}

	if (do_free && val_old) {
		if (map->params.free_val) {
			map->params.free_val((void*)val_old);
		} else {
			free((void*)val_old);
		}
	}

	return was_removed;
}

static size_t remove_all(const struct PPmap* const map) {
	memset(map->keys, 0, map->size * sizeof(void*));
	memset(map->vals, 0, map->size * sizeof(void*));

	size_t removed = map->size;

	((struct PPmap*)map)->size = 0;

	return removed;
}

static size_t remove_in(const struct PPmap* const map, const struct PPmap* const in, bool do_free) {
	size_t removed = 0;

	// values to free, no duplicates or nulls
	const void **freed = calloc(map->size, sizeof(void*));
	size_t nf = 0;

	for (const void **k = in->keys; k < in->keys + in->size; k++) {
		const void *val_old = NULL;
		if (remove_(&val_old, map, *k, false)) {
			removed++;

			if (val_old && do_free) {
				bool already_freed = false;
				for (const void **vf = freed; vf < freed + nf; vf++) {
					if (val_old == *vf) {
						already_freed = true;
						break;
					}
				}

				if (!already_freed) {
					map->params.free_val ? map->params.free_val((void*)val_old) : free((void*)val_old);
					*(freed + nf++) = val_old;
				}
			}
		}
	}

	free(freed);

	return removed;
}

static bool it_remove(const void **removed, const struct PPmapIt* const it, bool do_free) {
	if (removed)
		*removed = NULL;

	struct PPmapItState *st = it->st;
	if (!st) {
		ppmap_it_free(it);
		return false;
	}

	bool was_removed = remove_(removed, st->map, it->key, do_free);

	if (st->position > 0) {
		st->position--;
	} else {
		st->attached = false;
	}

	((struct PPmapIt*)it)->key = NULL;
	((struct PPmapIt*)it)->val = NULL;

	return was_removed;
}


static const struct Plist *vals_plist(const struct PPmap* const map, fn_clone clone_val) {
	const struct PlistParams params = {
		.equal_val = map->params.equal_val,
		.alloc_val = map->params.alloc_val,
		.free_val = map->params.free_val,
		.clone_val = map->params.clone_val,
		.str_val = map->params.str_val,
		.allow_null_val = map->params.allow_null_val,
		.initial = MAX(map->size, map->params.initial),
		.grow  = map->params.grow,
	};
	const struct Plist *list = plist_init_with(params);

	for (const void **v = map->vals; v < map->vals + map->size; v++) {
		if (*v && clone_val) {
			plist_append(list, (void*)clone_val(*v));
		} else {
			plist_append(list, (void*)*v);
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
	return from && from->params.clone_val ? clone(from, from->params.clone_val) : NULL;
}

void ppmap_free(const struct PPmap* const map) {
	if (!map)
		return;

	free_keys(map);

	remove_all(map);

	free(map->keys);
	free(map->vals);

	free((void*)map);
}

void ppmap_free_vals(const struct PPmap* const map) {
	if (!map)
		return;

	free_vals(map);

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

	for (const void **k = map->keys, **v = map->vals; k < map->keys + map->size; k++, v++) {
		if (map->params.equal_key ? map->params.equal_key(*k, key) : *k == key) {
			return *v;
		}
	}

	return NULL;
}

bool ppmap_contains_key(const struct PPmap* const map, const void* const key) {
	if (!map || !key)
		return false;

	for (const void **k = map->keys; k < map->keys + map->size; k++) {
		if (map->params.equal_key ? map->params.equal_key(*k, key) : *k == key) {
			return true;
		}
	}

	return false;
}

bool ppmap_contains_val(const struct PPmap* const map, const void* const val) {
	if (!map)
		return false;

	for (const void **v = map->vals; v < map->vals + map->size; v++) {
		if (map->params.equal_val ? map->params.equal_val(*v, val) : *v == val) {
			return true;
		}
	}

	return false;
}

const void *ppmap_first_key(const struct PPmap *const map, const void* const val) {
	if (!map)
		return NULL;

	for (const void **k = map->keys, **v = map->vals; k < map->keys + map->size; k++, v++) {
		if (map->params.equal_val ? map->params.equal_val(*v, val) : *v == val) {
			return *k;
		}
	}

	return NULL;
}

struct PPmapPair ppmap_at(const struct PPmap* const map, const size_t i) {
	struct PPmapPair res = { 0 };

	if (map && i < map->size) {
		res.key = *(map->keys + i);
		res.val = *(map->vals + i);
	}

	return res;
}

struct PPmapPair ppmap_find(const struct PPmap* const map, const struct PPmapFilter filter) {
	struct PPmapPair res = { 0 };

	if (!map)
		return res;

	for (const void **k = map->keys, **v = map->vals; k < map->keys + map->size; k++, v++) {
		if (!filter_blocks(&filter, *k, *v)) {
			res.key = *k;
			res.val = *v;

			return res;
		}
	}

	return res;
}

const struct PPmapIt *ppmap_it(const struct PPmap* const map) {
	return map ? ppmap_it_next(it_init(map, NULL)) : NULL;
}

const struct PPmapIt *ppmap_filter_it(const struct PPmap* const map, const struct PPmapFilter filter) {
	return map ? ppmap_it_next(it_init(map, &filter)) : NULL;
}

const struct PPmapIt *ppmap_it_next(const struct PPmapIt* const it) {
	if (!it)
		return NULL;

	struct PPmapItState *st = it->st;

	if (!it->st) {
		ppmap_it_free(it);
		return NULL;
	}

	if (st->attached) {
		st->position++;
	} else {
		st->position = 0;
	}
	st->attached = true;

	for ( ; st->position < st->map->size; st->position++) {

		struct PPmapIt *it_m = (struct PPmapIt*)it;

		it_m->key = *(st->map->keys + st->position);
		it_m->val = *(st->map->vals + st->position);

		if (filter_blocks(&st->filter, it_m->key, it_m->val))
			continue;

		return it;
	}

	ppmap_it_free(it);
	return NULL;
}


const void *ppmap_put(const struct PPmap* const map, const void* const key, const void* const val) {
	if (!map)
		return NULL;

	const void *val_old = NULL;
	put(&val_old, map, key, val, map->params.alloc_val, true);

	return val_old;
}

const void *ppmap_put_if_absent(const struct PPmap* const map, const void* const key, const void* const val) {
	if (!map)
		return NULL;

	const void *val_old = NULL;
	if (put(&val_old, map, key, val, map->params.alloc_val, false)) {
		return val_old;
	} else {
		return NULL;
	}
}

bool ppmap_put_free(const struct PPmap* const map, const void* const key, const void* const val) {
	if (!map)
		return false;

	const void *val_old = NULL;
	bool was_overwritten = put(&val_old, map, key, val, map->params.alloc_val, true);

	if (val_old) {
		map->params.free_val ? map->params.free_val((void*)val_old) : free((void*)val_old);
	}

	return was_overwritten;
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
	if (!map)
		return NULL;

	const void *removed = NULL;
	remove_(&removed, map, key, false);

	return removed;
}

bool ppmap_remove_free(const struct PPmap* const map, const void* const key) {
	return map ? remove_(NULL, map, key, true) : false;
}

size_t ppmap_remove_all(const struct PPmap* const map) {
	if (!map)
		return 0;

	free_keys(map);

	return remove_all(map);
}

size_t ppmap_remove_all_free(const struct PPmap* const map) {
	if (!map)
		return 0;

	free_keys(map);
	free_vals(map);

	return remove_all(map);
}

size_t ppmap_remove_in(const struct PPmap* const map, const struct PPmap* const in) {
	return map && in ? remove_in(map, in, false) : 0;
}

size_t ppmap_remove_in_free(const struct PPmap* const map, const struct PPmap* const in) {
	return map && in ? remove_in(map, in, true) : 0;
}

const void *ppmap_it_remove(const struct PPmapIt* const it) {
	if (!it)
		return NULL;

	const void *removed = NULL;
	it_remove(&removed, it, false);

	return removed;
}

bool ppmap_it_remove_free(const struct PPmapIt* const it) {
	return it ? it_remove(NULL, it, true) : false;
}

bool ppmap_equal(const struct PPmap* const a, const struct PPmap* const b) {
	if (!a || !b || a->size != b->size)
		return false;

	for (const void **bk = b->keys, **bv = b->vals; bk < b->keys + b->size; bk++, bv++) {
		const void *av = ppmap_get(a, *bk);
		if (a->params.equal_val) {
			if (!a->params.equal_val(av, *bv)) {
				return false;
			}
		} else if (av != *bv) {
			return false;
		}
	}

	return true;
}

bool ppmap_equal_ordered(const struct PPmap* const a, const struct PPmap* const b) {
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

const struct Plist *ppmap_keys_plist(const struct PPmap* const map) {
	if (!map)
		return NULL;

	const struct PlistParams params = {
		.equal_val = map->params.equal_key,
		.alloc_val = map->params.alloc_key,
		.free_val = map->params.free_key,
		.clone_val = map->params.alloc_key,
		.str_val = map->params.str_key,
		.initial = MAX(map->size, map->params.initial),
		.grow  = map->params.grow,
	};
	const struct Plist *list = plist_init_with(params);

	for (const void **k = map->keys; k < map->keys + map->size; k++) {
		plist_append(list, *k);
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

	for (const void **k = map->keys; k < map->keys + map->size; k++) {
		pset_add(set, *k);
	}

	return set;
}

const struct Plist *ppmap_vals_plist(const struct PPmap* const map) {
	return map ? vals_plist(map, NULL) : NULL;
}

const struct Plist *ppmap_vals_plist_clone(const struct PPmap* const map) {
	if (!map || !map->params.clone_val)
		return NULL;

	return vals_plist(map, map->params.clone_val);
}

char *ppmap_str(const struct PPmap* const map) {
	if (!map)
		return NULL;

	char *out = strdup("");

	for (const void **k = map->keys, **v = map->vals; k < map->keys + map->size; k++, v++) {

		if (map->params.str_key) {
			char *key_str = map->params.str_key(*k);
			if (key_str) {
				out = sprintf_append(out, "%s = ", key_str);
				free(key_str);
			} else {
				out = sprintf_append(out, "(null) = ");
			}
		} else {
			if (*k) {
				out = sprintf_append(out, "%p = ", *k);
			} else {
				out = sprintf_append(out, "(null) = ");
			}
		}

		if (map->params.str_val) {
			char *val_str = map->params.str_val(*v);
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

size_t ppmap_size(const struct PPmap* const map) {
	return map ? map->size : 0;
}
