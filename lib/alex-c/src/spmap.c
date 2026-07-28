#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "fn.h"
#include "ppmap.h"
#include "slist.h"
#include "sset.h"

#include "spmap.h"

struct SPmap {
	const struct SPmapParams params;
	const struct PPmap *ppmap;
};

struct SPmapItState {
	const struct PPmapIt *pit;
};

static const struct SPmap *clone(const struct SPmap* const from, bool deep) {
	if (!from)
		return NULL;

	struct SPmap *to = calloc(1, sizeof(struct SPmap));

	to->ppmap = deep ? ppmap_clone_deep(from->ppmap) : ppmap_clone(from->ppmap) ;

	memcpy((void*)&to->params, &from->params, sizeof(struct SPmapParams));

	return to;
}

static struct PPmapFilter ppmap_filter_init(const struct SPmapFilter *filter) {
	const struct PPmapFilter ppmap_filter = {
		.key = (fn_pred_p)filter->key,
		.val = filter->val,
		.key_val = (fn_pred_pp)filter->key_val,
		.data = filter->data,
		.key_data = (fn_pred_pp)filter->key_data,
		.val_data = filter->val_data,
		.key_val_data = (fn_pred_ppp)filter->key_val_data,
	};

	return ppmap_filter;
}

static const struct SPmapIt *it_init(const struct PPmapIt *pit) {
	if (!pit)
		return NULL;

	struct SPmapIt *it = calloc(1, sizeof(struct SPmapIt));
	it->st = calloc(1, sizeof(struct SPmapItState));

	it->st->pit = pit;
	it->key = pit->key;
	it->val = pit->val;

	return it;
}

const struct SPmap *spmap_init(void) {
	const struct SPmapParams params = { 0 };
	return spmap_init_with(params);
}

const struct SPmap *spmap_init_with(const struct SPmapParams params) {
	const struct PPmapParams ppmap_params = {
		.equal_key = params.case_insensitive_key ? (fn_equal)equal_strcasecmp : (fn_equal)equal_strcmp,
		.equal_val = params.equal_val,
		.alloc_key = (fn_clone)clone_strdup,
		.alloc_val = params.alloc_val,
		.free_key = free,
		.free_val = params.free_val,
		.clone_val = params.clone_val,
		.str_key = (fn_str)str_or_null,
		.str_val = params.str_val,
		.allow_null_val = params.allow_null_val,
		.initial = params.initial,
		.grow = params.grow,
	};

	struct SPmap *map =  calloc(1, sizeof(struct SPmap));
	map->ppmap = ppmap_init_with(ppmap_params);;
	memcpy((void*)&map->params, &params, sizeof(struct SPmapParams));

	return map;
}

const struct SPmap *spmap_clone(const struct SPmap* const from) {
	return clone(from, false);
}

const struct SPmap *spmap_clone_deep(const struct SPmap* const from) {
	return clone(from, true);
}

void spmap_free(const struct SPmap* const map) {
	if (!map)
		return;

	ppmap_free(map->ppmap);

	free((void*)map);
}

void spmap_free_vals(const struct SPmap* const map) {
	if (!map)
		return;

	ppmap_free_vals(map->ppmap);

	free((void*)map);
}

void spmap_it_free(const struct SPmapIt* const it) {
	if (!it)
		return;

	if (it->st) {
		ppmap_it_free(it->st->pit);
	}

	free(it->st);
	free((void*)it);
}

const void *spmap_get(const struct SPmap* const map, const char* const key) {
	return map ? ppmap_get(map->ppmap, key) : NULL;
}

bool spmap_contains_key(const struct SPmap* const map, const char* const key) {
	return map ? ppmap_contains_key(map->ppmap, key) : false;
}

bool spmap_contains_val(const struct SPmap* const map, const void* const val) {
	return map ? ppmap_contains_val(map->ppmap, val) : false;
}

const char *spmap_first_key(const struct SPmap *const map, const void* const val) {
	return map ? ppmap_first_key(map->ppmap, val) : false;
}

struct SPmapPair spmap_at(const struct SPmap* const map, const size_t i) {
	struct SPmapPair res = { 0 };

	if (!map)
		return res;

	struct PPmapPair pres = ppmap_at(map->ppmap, i);

	res.key = pres.key;
	res.val = pres.val;

	return res;
}

struct SPmapPair spmap_find(const struct SPmap* const map, const struct SPmapFilter filter) {
	struct SPmapPair res = { 0 };

	if (!map)
		return res;

	struct PPmapPair pres = ppmap_find(map->ppmap, ppmap_filter_init(&filter));

	res.key = pres.key;
	res.val = pres.val;

	return res;
}

const struct SPmapIt *spmap_it(const struct SPmap* const map) {
	return map ? it_init(ppmap_it(map->ppmap)) : NULL;
}

const struct SPmapIt *spmap_filter_it(const struct SPmap* const map, const struct SPmapFilter filter) {
	return map ? it_init(ppmap_filter_it(map->ppmap, ppmap_filter_init(&filter))) : NULL;
}

const struct SPmapIt *spmap_it_next(const struct SPmapIt* const it) {
	if (!it)
		return NULL;


	if (!it->st) {
		spmap_it_free(it);
		return NULL;
	}

	it->st->pit = ppmap_it_next(it->st->pit);

	if (it->st->pit) {
		struct SPmapIt *it_m = (struct SPmapIt*)it;
		it_m->key = it->st->pit->key;
		it_m->val = it->st->pit->val;
		return it;
	} else {
		spmap_it_free(it);
		return NULL;
	}
}

const void *spmap_put(const struct SPmap* const map, const char* const key, const void* const val) {
	return map ? ppmap_put(map->ppmap, key, val) : NULL;
}

bool spmap_put_free(const struct SPmap* const map, const char* const key, const void* const val) {
	return map ? ppmap_put_free(map->ppmap, key, val) : false;
}

const void *spmap_put_clone(const struct SPmap* const map, const char* const key, const void* const val) {
	return map && map->params.clone_val ? ppmap_put_clone(map->ppmap, key, val) : NULL;
}

bool spmap_put_clone_free(const struct SPmap* const map, const char* const key, const void* const val) {
	return map && map->params.clone_val ? ppmap_put_clone_free(map->ppmap, key, val) : false;
}

const void *spmap_put_if_absent(const struct SPmap* const map, const char* const key, const void* const val) {
	return map ? ppmap_put_if_absent(map->ppmap, key, val) : NULL;
}

const void *spmap_put_if_absent_clone(const struct SPmap* const map, const char* const key, const void* const val) {
	return map && map->params.clone_val ? ppmap_put_if_absent_clone(map->ppmap, key, val) : NULL;
}

const void *spmap_remove(const struct SPmap* const map, const char* const key) {
	return map ? ppmap_remove(map->ppmap, key) : NULL;
}

bool spmap_remove_free(const struct SPmap* const map, const char* const key) {
	return map ? ppmap_remove_free(map->ppmap, key) : false;
}

size_t spmap_remove_all(const struct SPmap* const map) {
	return map ? ppmap_remove_all(map->ppmap) : false;
}

size_t spmap_remove_all_free(const struct SPmap* const map) {
	return map ? ppmap_remove_all_free(map->ppmap) : false;
}

size_t spmap_remove_in(const struct SPmap* const map, const struct SPmap* const in) {
	return map && in ? ppmap_remove_in(map->ppmap, in->ppmap) : 0;
}

size_t spmap_remove_in_free(const struct SPmap* const map, const struct SPmap* const in) {
	return map && in ? ppmap_remove_in_free(map->ppmap, in->ppmap) : 0;
}

const void *spmap_it_remove(const struct SPmapIt* const it) {
	if (!it)
		return NULL;

	if (!it->st) {
		spmap_it_free(it);
		return NULL;
	}

	((struct SPmapIt*)it)->key = NULL;
	((struct SPmapIt*)it)->val = NULL;

	return ppmap_it_remove(it->st->pit);
}

bool spmap_it_remove_free(const struct SPmapIt* const it) {
	if (!it)
		return false;

	if (!it->st) {
		spmap_it_free(it);
		return false;
	}

	((struct SPmapIt*)it)->key = NULL;
	((struct SPmapIt*)it)->val = NULL;

	return ppmap_it_remove_free(it->st->pit);
}

size_t spmap_put_all(const struct SPmap* const map, const struct SPmap* const from) {
	return map && from ? ppmap_put_all(map->ppmap, from->ppmap) : 0;
}

size_t spmap_put_all_free(const struct SPmap* const map, const struct SPmap* const from) {
	return map && from ? ppmap_put_all_free(map->ppmap, from->ppmap) : 0;
}

size_t spmap_put_all_clone(const struct SPmap* const map, const struct SPmap* const from) {
	return map && from ? ppmap_put_all_clone(map->ppmap, from->ppmap) : 0;
}

size_t spmap_put_all_clone_free(const struct SPmap* const map, const struct SPmap* const from) {
	return map && from ? ppmap_put_all_clone_free(map->ppmap, from->ppmap) : 0;
}

bool spmap_equal(const struct SPmap* const a, const struct SPmap* const b) {
	return a && b ? ppmap_equal(a->ppmap, b->ppmap) : false;
}

bool spmap_equal_ordered(const struct SPmap* const a, const struct SPmap* const b) {
	return a && b ? ppmap_equal_ordered(a->ppmap, b->ppmap) : false;
}

const struct Slist *spmap_keys_slist(const struct SPmap* const map) {
	if (!map)
		return NULL;

	const struct SlistParams params = {
		.case_insensitive = map->params.case_insensitive_key,
		.initial = MAX(ppmap_size(map->ppmap), map->params.initial),
		.grow = map->params.grow,
	};
	const struct Slist *list = slist_init_with(params);

	for (const struct SPmapIt *it = spmap_it(map); it; it = spmap_it_next(it)) {
		slist_append(list, it->key);
	}

	return list;
}

const struct Sset *spmap_keys_sset(const struct SPmap* const map) {
	if (!map)
		return NULL;

	const struct SsetParams params = {
		.case_insensitive = map->params.case_insensitive_key,
		.initial = MAX(ppmap_size(map->ppmap), map->params.initial),
		.grow = map->params.grow,
	};
	const struct Sset *set = sset_init_with(params);

	for (const struct SPmapIt *it = spmap_it(map); it; it = spmap_it_next(it)) {
		sset_add(set, it->key);
	}

	return set;
}

const struct Plist *spmap_vals_plist(const struct SPmap* const map) {
	return map ? ppmap_vals_plist(map->ppmap) : NULL;
}

const struct Plist *spmap_vals_plist_clone(const struct SPmap* const map) {
	return map ? ppmap_vals_plist_clone(map->ppmap) : NULL;
}

const struct Pset *spmap_vals_pset(const struct SPmap* const map) {
	return map ? ppmap_vals_pset(map->ppmap) : NULL;
}

const struct Pset *spmap_vals_pset_clone(const struct SPmap* const map) {
	return map ? ppmap_vals_pset_clone(map->ppmap) : NULL;
}

char *spmap_str(const struct SPmap* const map) {
	return map ? ppmap_str(map->ppmap) : NULL;
}

size_t spmap_size(const struct SPmap* const map) {
	return map ? ppmap_size(map->ppmap) : 0;
}
