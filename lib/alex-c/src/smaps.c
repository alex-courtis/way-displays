#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "fn.h"
#include "pmap.h"
#include "sset.h"

#include "smaps.h"

struct SMapS {
	const struct SMapSParams params;
	const struct PMap *pmap;
};

struct SMapSItState {
	const struct PMapIt *pit;
};

static const struct SMapSIt *it_init(const struct PMapIt *pit) {
	if (!pit)
		return NULL;

	struct SMapSIt *it = calloc(1, sizeof(struct SMapSIt));
	it->st = calloc(1, sizeof(struct SMapSItState));

	it->st->pit = pit;
	it->key = pit->key;
	it->val = pit->val;

	return it;
}

const struct SMapS *smaps_init(void) {
	const struct SMapSParams params = { 0 };
	return smaps_init_with(params);
}

const struct SMapS *smaps_init_with(const struct SMapSParams params) {
	const struct PMapParams pmap_params = {
		.equal_key = params.case_insensitive_key ? (fn_equal)equal_strcasecmp : (fn_equal)equal_strcmp,
		.equal_val = params.case_insensitive_val ? (fn_equal)equal_strcasecmp : (fn_equal)equal_strcmp,
		.alloc_key = clone_strdup,
		.alloc_val = clone_strdup,
		.free_key = (fn_free)free,
		.free_val = (fn_free)free,
		.clone_val = clone_strdup,
		.str_key = (fn_str)str_or_null,
		.str_val = (fn_str)str_or_null,
		.initial = params.initial,
		.allow_null_val = params.allow_null_val,
		.grow = params.grow,
	};

	struct SMapS *map = calloc(1, sizeof(struct SMapS));
	map->pmap = pmap_init_with(pmap_params);;
	memcpy((void*)&map->params, &params, sizeof(struct SMapSParams));

	return map;
}

const struct SMapS *smaps_clone(const struct SMapS* const from) {
	if (!from)
		return NULL;

	struct SMapS *to = calloc(1, sizeof(struct SMapS));
	to->pmap = pmap_clone_deep(from->pmap);
	memcpy((void*)&to->params, &from->params, sizeof(struct SMapSParams));

	return to;
}

void smaps_free(const struct SMapS* const map) {
	if (!map)
		return;

	pmap_free_vals(map->pmap);

	free((void*)map);
}

void smaps_it_free(const struct SMapSIt* const it) {
	if (!it)
		return;

	if (it->st) {
		pmap_it_free(it->st->pit);
	}

	free(it->st);
	free((void*)it);
}

const char *smaps_get(const struct SMapS* const map, const char* const key) {
	return map ? pmap_get(map->pmap, key) : NULL;
}

bool smaps_contains_key(const struct SMapS* const map, const char* const key) {
	return map ? pmap_contains_key(map->pmap, key) : false;
}

bool smaps_contains_val(const struct SMapS* const map, const char* const val) {
	return map ? pmap_contains_val(map->pmap, val) : false;
}

struct SMapSPair smaps_match(const struct SMapS* const map, fn_match_str_str match, const void* const data) {
	struct SMapSPair res = { 0 };

	if (!map)
		return res;

	struct PMapPair pres = pmap_match(map->pmap, (fn_match_ptr_ptr)match, data);

	res.key = pres.key;
	res.val = pres.val;

	return res;
}

struct SMapSPair smaps_match_val(const struct SMapS* const map, fn_match_str match, const void* const data) {
	struct SMapSPair res = { 0 };

	if (!map)
		return res;

	struct PMapPair pres = pmap_match_val(map->pmap, (fn_match_ptr)match, data);

	res.key = pres.key;
	res.val = pres.val;

	return res;
}

const struct SMapSIt *smaps_it(const struct SMapS* const map) {
	return map ? it_init(pmap_it(map->pmap)) : NULL;
}

const struct SMapSIt *smaps_match_it(const struct SMapS* const map, fn_match_str_str match, const void* const data) {
	return map ? it_init(pmap_match_it(map->pmap, (fn_match_ptr_ptr)match, data)) : NULL;
}

const struct SMapSIt *smaps_match_val_it(const struct SMapS* const map, fn_match_str match, const void* const data) {
	return map ? it_init(pmap_match_val_it(map->pmap, (fn_match_ptr)match, data)) : NULL;
}

const struct SMapSIt *smaps_it_next(const struct SMapSIt* const it) {
	if (!it)
		return NULL;


	if (!it->st) {
		smaps_it_free(it);
		return NULL;
	}

	it->st->pit = pmap_it_next(it->st->pit);

	if (it->st->pit) {
		struct SMapSIt *it_m = (struct SMapSIt*)it;
		it_m->key = it->st->pit->key;
		it_m->val = it->st->pit->val;
		return it;
	} else {
		smaps_it_free(it);
		return NULL;
	}
}

bool smaps_put(const struct SMapS* const map, const char* const key, const char* const val) {
	return map ? pmap_put_free(map->pmap, key, val): false;
}

bool smaps_put_if_absent(const struct SMapS* const map, const char* const key, const char* const val) {
	return map ? pmap_put_if_absent(map->pmap, key, val) : false;
}

bool smaps_remove(const struct SMapS* const map, const char* const key) {
	return map ? pmap_remove_free(map->pmap, key) : false;
}

bool smaps_equal(const struct SMapS* const a, const struct SMapS* const b) {
	return a && b ? pmap_equal(a->pmap, b->pmap) : false;
}

struct SList *smaps_keys_slist_deep(const struct SMapS* const map) {
	return map ? pmap_keys_slist_deep(map->pmap) : NULL;
}

const struct SSet *smaps_keys_sset(const struct SMapS* const map) {
	if (!map)
		return NULL;

	const struct SSetParams params = {
		.case_insensitive = map->params.case_insensitive_key,
		.initial = MAX(pmap_size(map->pmap), map->params.initial),
		.grow = map->params.grow,
	};
	const struct SSet *set = sset_init_with(params);

	for (const struct SMapSIt *it = smaps_it(map); it; it = smaps_it_next(it)) {
		sset_add(set, it->key);
	}

	return set;
}

struct SList *smaps_vals_slist_deep(const struct SMapS* const map) {
	return map ? pmap_vals_slist_deep(map->pmap) : NULL;
}

const struct SSet *smaps_vals_sset(const struct SMapS* const map) {
	if (!map)
		return NULL;

	const struct SSetParams params = {
		.case_insensitive = map->params.case_insensitive_val,
		.initial = MAX(pmap_size(map->pmap), map->params.initial),
		.grow = map->params.grow,
	};
	const struct SSet *set = sset_init_with(params);

	for (const struct SMapSIt *it = smaps_it(map); it; it = smaps_it_next(it)) {
		sset_add(set, it->val);
	}

	return set;
}

char *smaps_str(const struct SMapS* const map) {
	return map ? pmap_str(map->pmap) : NULL;
}

size_t smaps_size(const struct SMapS* const map) {
	return map ? pmap_size(map->pmap) : 0;
}
