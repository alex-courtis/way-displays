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

struct IMapMatchData {
	fn_match_size_t_ptr match_key_val;
	fn_match_size_t match_key;
	fn_match_ptr match_val;
	const void *data;
};

struct IMapItState {
	const struct PMapIt *pit;
	const struct IMapMatchData *match_data;
};

static bool equal_key_size_t(const void* const a, const void* const b) {
	if (!a || !b)
		return false;

	return *(size_t*)a == *(size_t*)b;
}

static void *alloc_key_size_t(const void* const val) {
	size_t *out = calloc(1, sizeof(size_t));
	*out = *(size_t*)val;

	return out;
}

static char *str_key_size_t(const void* const val) {
	return sprintf_alloc("%zu", *(size_t*)val);
}

static bool match_key_val_wrapper(const void* const key, const void* const val, const void* const data) {
	const struct IMapMatchData* const matcher = data;
	return matcher->match_key_val(*(size_t*)key, val, matcher->data);
}

static bool match_key_wrapper(const void* const val, const void* const data) {
	const struct IMapMatchData* const matcher = data;
	return matcher->match_key(*(size_t*)val, matcher->data);
}

static bool match_val_wrapper(const void* const val, const void* const data) {
	const struct IMapMatchData* const matcher = data;
	return matcher->match_val(val, matcher->data);
}

static const struct IMap *clone(const struct IMap* const from, bool deep) {
	if (!from)
		return NULL;

	struct IMap *to = calloc(1, sizeof(struct IMap));

	to->pmap = deep ? pmap_clone_deep(from->pmap) : pmap_clone_shallow(from->pmap);

	memcpy((void*)&to->params, &from->params, sizeof(struct IMapParams));

	return to;
}

static struct IMapIt *it_init(const struct PMapIt *pit) {
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
		.equal_key = equal_key_size_t,
		.equal_val = params.equal_val,
		.alloc_key = alloc_key_size_t,
		.alloc_val = params.alloc_val,
		.free_key = (fn_free)free,
		.free_val = params.free_val,
		.clone_val = params.clone_val,
		.str_key = str_key_size_t,
		.str_val = params.str_val,
		.allow_null_val = params.allow_null_val,
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

bool imap_contains_val(const struct IMap* const map, const void* const val) {
	return map ? pmap_contains_val(map->pmap, val) : false;
}

struct IMapPair imap_match(const struct IMap* const map, fn_match_size_t_ptr match, const void* const data) {
	struct IMapPair res = { 0 };

	if (!map || !match)
		return res;

	struct IMapMatchData match_data = {
		.match_key_val = match,
		.data = data,
	};

	struct PMapPair pres = pmap_match(map->pmap, match_key_val_wrapper, &match_data);

	res.key = pres.key ? *(size_t*)pres.key : 0;
	res.val = pres.val;

	return res;
}

struct IMapPair imap_match_key(const struct IMap* const map, fn_match_size_t match, const void* const data) {
	struct IMapPair res = { 0 };

	if (!map || !match)
		return res;

	struct IMapMatchData match_data = {
		.match_key = match,
		.data = data,
	};

	struct PMapPair pres = pmap_match_key(map->pmap, match_key_wrapper, &match_data);

	res.key = pres.key ? *(size_t*)pres.key : 0;
	res.val = pres.val;

	return res;
}

struct IMapPair imap_match_val(const struct IMap* const map, fn_match_ptr match, const void* const data) {
	struct IMapPair res = { 0 };

	if (!map || !match)
		return res;

	struct IMapMatchData match_data = {
		.match_val = match,
		.data = data,
	};

	struct PMapPair pres = pmap_match_val(map->pmap, match_val_wrapper, &match_data);

	res.key = pres.key ? *(size_t*)pres.key : 0;
	res.val = pres.val;

	return res;
}

const struct IMapIt *imap_it(const struct IMap* const map) {
	return map ? it_init(pmap_it(map->pmap)) : NULL;
}

const struct IMapIt *imap_match_it(const struct IMap* const map, fn_match_size_t_ptr match, const void* const data) {
	if (!map || !match)
		return NULL;

	struct IMapMatchData *match_data = calloc(1, sizeof(struct IMapMatchData));
	match_data->match_key_val = match;
	match_data->data = data;

	struct IMapIt *it = it_init(pmap_match_it(map->pmap, match_key_val_wrapper, match_data));

	if (it) {
		it->st->match_data = match_data;
		return it;
	} else {
		free(match_data);
		return NULL;
	}
}

const struct IMapIt *imap_match_val_it(const struct IMap* const map, fn_match_ptr match, const void* const data) {
	if (!map || !match)
		return NULL;

	struct IMapMatchData *match_data = calloc(1, sizeof(struct IMapMatchData));
	match_data->match_val = match;
	match_data->data = data;

	struct IMapIt *it = it_init(pmap_match_val_it(map->pmap, match_val_wrapper, match_data));

	if (it) {
		it->st->match_data = match_data;
		return it;
	} else {
		free(match_data);
		return NULL;
	}
}

const struct IMapIt *imap_it_next(const struct IMapIt* const it) {
	if (!it)
		return NULL;


	if (!it->st) {
		imap_it_free(it);
		return NULL;
	}

	it->st->pit = pmap_it_next(it->st->pit);

	if (it->st->pit) {
		struct IMapIt *it_m = (struct IMapIt*)it;
		it_m->key = *(size_t*)it->st->pit->key;
		it_m->val = it->st->pit->val;
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

const struct PSet *imap_vals_pset(const struct IMap* const map) {
	return map ? pmap_vals_pset(map->pmap) : NULL;
}

char *imap_str(const struct IMap* const map) {
	return map ? pmap_str(map->pmap) : NULL;
}

size_t imap_size(const struct IMap* const map) {
	return map ? pmap_size(map->pmap) : 0;
}
