#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "pmap.h"

#include "smap.h"

struct SMap {
	const struct SMapParams params;
	const struct PMap *pmap;
};

struct SMapItState {
	const struct PMapIt *pit;
};

const struct SMap *smap_init(void) {
	const struct SMapParams params = { 0 };
	return smap_init_with(params);
}

static const struct SMap *clone(const struct SMap* const from, bool deep) {
	if (!from)
		return NULL;

	struct SMap *to = calloc(1, sizeof(struct SMap));

	to->pmap = deep ? pmap_clone_deep(from->pmap) : pmap_clone_shallow(from->pmap) ;

	memcpy((void*)&to->params, &from->params, sizeof(struct SMapParams));

	return to;
}

static const struct SMapIt *it_init(const struct PMapIt *pit) {
	if (!pit)
		return NULL;

	struct SMapIt *it = calloc(1, sizeof(struct SMapIt));
	it->st = calloc(1, sizeof(struct SMapItState));

	it->st->pit = pit;
	it->key = pit->key;
	it->val = pit->val;

	return it;
}

const struct SMap *smap_init_with(const struct SMapParams params) {
	const struct PMapParams pmap_params = {
		.equal_key = params.case_insensitive ? (fn_equal)fn_equal_strcasecmp : (fn_equal)fn_equal_strcmp,
		.equal_val = params.equal_val,
		.alloc_key = fn_clone_strdup,
		.alloc_val = params.alloc_val,
		.free_key = (fn_free)free,
		.free_val = params.free_val,
		.clone_val = params.clone_val,
		.str_key = (fn_str)fn_str_or_null,
		.str_val = params.str_val,
		.initial = params.initial,
		.grow = params.grow,
	};

	struct SMap *map =  calloc(1, sizeof(struct SMap));
	map->pmap = pmap_init_with(pmap_params);;
	memcpy((void*)&map->params, &params, sizeof(struct SMapParams));

	return map;
}

const struct SMap *smap_clone_shallow(const struct SMap* const from) {
	return clone(from, false);
}

const struct SMap *smap_clone_deep(const struct SMap* const from) {
	return clone(from, true);
}

void smap_free(const struct SMap* const map) {
	if (!map)
		return;

	pmap_free(map->pmap);

	free((void*)map);
}

void smap_free_vals(const struct SMap* const map) {
	if (!map)
		return;

	pmap_free_vals(map->pmap);

	free((void*)map);
}

void smap_it_free(const struct SMapIt* const it) {
	if (!it)
		return;

	if (it->st) {
		pmap_it_free(it->st->pit);
	}

	free(it->st);
	free((void*)it);
}

const void *smap_get(const struct SMap* const map, const char* const key) {
	return map ? pmap_get(map->pmap, key) : NULL;
}

bool smap_contains_key(const struct SMap* const map, const char* const key) {
	return map ? pmap_contains_key(map->pmap, key) : false;
}

struct SMapPair smap_match(const struct SMap* const map, fn_match_smap match, const void* const data) {
	struct SMapPair res = { 0 };

	if (!map)
		return res;

	struct PMapPair pres = pmap_match(map->pmap, (fn_match_key_val)match, data);

	res.key = pres.key;
	res.val = pres.val;

	return res;
}

const struct SMapIt *smap_it(const struct SMap* const map) {
	return map ? it_init(pmap_it(map->pmap)) : NULL;
}

const struct SMapIt *smap_match_it(const struct SMap* const map, fn_match_smap match, const void* const data) {
	return map ? it_init(pmap_match_it(map->pmap, (fn_match_key_val)match, data)) : NULL;
}

const struct SMapIt *smap_it_next(const struct SMapIt* const it) {
	if (!it)
		return NULL;


	if (!it->st) {
		smap_it_free(it);
		return NULL;
	}

	it->st->pit = pmap_it_next(it->st->pit);

	if (it->st->pit) {
		struct SMapIt *it_m = (struct SMapIt*)it;
		it_m->key = it->st->pit->key;
		it_m->val = it->st->pit->val;
		return it;
	} else {
		smap_it_free(it);
		return NULL;
	}
}

const void *smap_put(const struct SMap* const map, const char* const key, const void* const val) {
	return map ? pmap_put(map->pmap, key, val) : NULL;
}

const void *smap_put_if_absent(const struct SMap* const map, const char* const key, const void* const val) {
	return map ? pmap_put_if_absent(map->pmap, key, val) : NULL;
}

bool smap_put_free(const struct SMap* const map, const  char* const key, const void* const val) {
	return map ? pmap_put_free(map->pmap, key, val) : false;
}

const void *smap_remove(const struct SMap* const map, const char* const key) {
	return map ? pmap_remove(map->pmap, key) : NULL;
}

bool smap_remove_free(const struct SMap* const map, const char* const key) {
	return map ? pmap_remove_free(map->pmap, key) : false;
}

bool smap_equal(const struct SMap* const a, const struct SMap* const b) {
	return a && b ? pmap_equal(a->pmap, b->pmap) : false;
}

struct SList *smap_keys_slist_deep(const struct SMap* const map) {
	return map ? pmap_keys_slist_deep(map->pmap) : NULL;
}

struct SList *smap_vals_slist_shallow(const struct SMap* const map) {
	return map ? pmap_vals_slist_shallow(map->pmap) : NULL;
}

struct SList *smap_vals_slist_deep(const struct SMap* const map) {
	return map ? pmap_vals_slist_deep(map->pmap) : NULL;
}

char *smap_str(const struct SMap* const map) {
	return map ? pmap_str(map->pmap) : NULL;
}

size_t smap_size(const struct SMap* const map) {
	return map ? pmap_size(map->pmap) : 0;
}
