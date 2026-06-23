#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "pmap.h"
#include "str.h"

#include "imap.h"

struct IMap {
	const struct IMapParams params;
	const struct PMap *pmap;
};

struct IMapItMatchData {
	fn_match_imap match;
	const void *data;
};

struct IMapItState {
	const struct PMapIt *pit;
	const struct IMapItMatchData *match_data;
};

static bool fn_equal_key(const void* const a, const void* const b) {
	if (!a || !b)
		return false;

	return *(size_t*)a == *(size_t*)b;
}

static void *fn_clone_key(const void* const val) {
	size_t *out = calloc(1, sizeof(size_t));
	*out = *(size_t*)val;

	return out;
}

static char *fn_str_key(const void* const val) {
	return sprintf_alloc("%zu", *(size_t*)val);
}

static bool fn_match_data_wrapper(const void* const key, const void* const val, const void* const data) {
	const struct IMapItMatchData* const matcher = data;
	return matcher->match(*(size_t*)key, val, matcher->data);
}

static const struct IMap *clone(const struct IMap* const from, bool deep) {
	if (!from)
		return NULL;

	struct IMap *to = calloc(1, sizeof(struct IMap));

	to->pmap = deep ? pmap_clone_deep(from->pmap) : pmap_clone_shallow(from->pmap);

	memcpy((void*)&to->params, &from->params, sizeof(struct IMapParams));

	return to;
}

static struct IMapIt *it_init(const struct IMap *map, const struct PMapIt *pit) {
	if (!pit)
		return NULL;

	struct IMapIt *it = calloc(1, sizeof(struct IMapIt));
	it->st = calloc(1, sizeof(struct IMapItState));

	it->st->pit = pit;
	it->key = *(size_t*)pit->key;
	it->val = pit->val;

	return it;
}

const struct IMap *imap_init(void) {
	const struct IMapParams params = { 0 };
	return imap_init_with(params);
}

const struct IMap *imap_init_with(const struct IMapParams params) {
	const struct PMapParams pmap_params = {
		.equal_key = fn_equal_key,
		.equal_val = params.equal_val,
		.alloc_key = fn_clone_key,
		.alloc_val = params.alloc_val,
		.free_key = (fn_free)free,
		.free_val = params.free_val,
		.clone_val = params.clone_val,
		.str_key = fn_str_key,
		.str_val = params.str_val,
		.initial = params.initial,
		.grow = params.grow,
	};

	struct IMap *map =  calloc(1, sizeof(struct IMap));
	map->pmap = pmap_init_with(pmap_params);;
	memcpy((void*)&map->params, &params, sizeof(struct IMapParams));

	return map;
}

const struct IMap *imap_clone_shallow(const struct IMap* const from) {
	return clone(from, false);
}

const struct IMap *imap_clone_deep(const struct IMap* const from) {
	return clone(from, true);
}

void imap_free(const struct IMap* const map) {
	if (!map)
		return;

	pmap_free(map->pmap);

	free((void*)map);
}

void imap_free_vals(const struct IMap* const map) {
	if (!map)
		return;

	pmap_free_vals(map->pmap);

	free((void*)map);
}

void imap_it_free(const struct IMapIt* const it) {
	if (!it)
		return;

	if (it->st) {
		free((void*)it->st->match_data);
		pmap_it_free(it->st->pit);
	}

	free(it->st);
	free((void*)it);
}

const void *imap_get(const struct IMap* const map, const size_t key) {
	return map ? pmap_get(map->pmap, &key) : NULL;
}

bool imap_contains_key(const struct IMap* const map, const size_t key) {
	return map ? pmap_contains_key(map->pmap, &key) : false;
}

struct IMapPair imap_match(const struct IMap* const map, fn_match_imap match, const void* const data) {
	struct IMapPair res = { 0 };

	if (!map || !match)
		return res;

	struct IMapItMatchData match_data = {
		.match = match,
		.data = data,
	};

	struct PMapPair pres = pmap_match(map->pmap, fn_match_data_wrapper, &match_data);

	res.key = pres.key ? *(size_t*)pres.key : 0;
	res.val = pres.val;

	return res;
}

const struct IMapIt *imap_it(const struct IMap* const map) {
	return map ? it_init(map, pmap_it(map->pmap)) : NULL;
}

const struct IMapIt *imap_match_it(const struct IMap* const map, fn_match_imap match, const void* const data) {
	if (!map || !match)
		return NULL;

	struct IMapItMatchData *match_data = calloc(1, sizeof(struct IMapItMatchData));
	match_data->match = match;
	match_data->data = data;

	struct IMapIt *it = it_init(map, pmap_match_it(map->pmap, fn_match_data_wrapper, match_data));

	if (it) {
		it->st->match_data = match_data;
		return it;
	} else {
		free(match_data);
		return NULL;
	}
}

const struct IMapIt *imap_it_next(const struct IMapIt* const cit) {
	if (!cit)
		return NULL;

	struct IMapIt *it = (struct IMapIt*)cit;

	if (!it->st) {
		imap_it_free(it);
		return NULL;
	}

	it->st->pit = pmap_it_next(cit->st->pit);

	if (it->st->pit) {
		it->key = *(size_t*)it->st->pit->key;
		it->val = it->st->pit->val;
		return it;
	} else {
		imap_it_free(it);
		return NULL;
	}
}

const void *imap_put(const struct IMap* const map, const size_t key, const void* const val) {
	return map ? pmap_put(map->pmap, &key, val) : NULL;
}

const void *imap_put_if_absent(const struct IMap* const map, const size_t key, const void* const val) {
	return map ? pmap_put_if_absent(map->pmap, &key, val) : NULL;
}

bool imap_put_free(const struct IMap* const map, const size_t key, const char* const val) {
	return map ? pmap_put_free(map->pmap, &key, val) : false;
}

const void *imap_remove(const struct IMap* const map, const size_t key) {
	return map ? pmap_remove(map->pmap, &key) : NULL;
}

bool imap_remove_free(const struct IMap* const map, const size_t key) {
	return map ? pmap_remove_free(map->pmap, &key) : false;
}

bool imap_equal(const struct IMap* const a, const struct IMap* const b) {
	return a && b ? pmap_equal(a->pmap, b->pmap) : false;
}

struct SList *imap_vals_slist_shallow(const struct IMap* const map) {
	return map ? pmap_vals_slist_shallow(map->pmap) : NULL;
}

struct SList *imap_vals_slist_deep(const struct IMap* const map) {
	return map ? pmap_vals_slist_deep(map->pmap) : NULL;
}

char *imap_str(const struct IMap* const map) {
	return map ? pmap_str(map->pmap) : NULL;
}

size_t imap_size(const struct IMap* const map) {
	return map ? pmap_size(map->pmap) : 0;
}
