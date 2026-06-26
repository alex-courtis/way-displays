#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "pmap.h"
#include "str.h"

#include "smapi.h"

struct SMapI {
	const struct SMapIParams params;
	const struct PMap *pmap;
};

struct SMapIItMatchData {
	fn_match_smapi match;
	const void *data;
};

struct SMapIItState {
	const struct PMapIt *pit;
	const struct SMapIItMatchData *match_data;
};

static bool equal_val_size_t(const void* const a, const void* const b) {
	return *(size_t*)a == *(size_t*)b;
}

static void *clone_val_size_t(const void* const val) {
	size_t *out = calloc(1, sizeof(size_t));
	*out = *(size_t*)val;

	return out;
}

static char *str_val_size_t(const void* const val) {
	return sprintf_alloc("%zu", *(size_t*)val);
}

static bool match_key_val_wrapper(const void* const key, const void* const val, const void* const data) {
	const struct SMapIItMatchData* const matcher = data;
	return matcher->match(key, *(size_t*)val, matcher->data);
}

static struct SMapIIt *it_init(const struct PMapIt *pit) {
	if (!pit)
		return NULL;

	struct SMapIIt *it = calloc(1, sizeof(struct SMapIIt));
	it->st = calloc(1, sizeof(struct SMapIItState));

	it->st->pit = pit;
	it->key = pit->key;
	it->val = *(size_t*)pit->val;

	return it;
}

const struct SMapI *smapi_init(void) {
	const struct SMapIParams params = { 0 };
	return smapi_init_with(params);
}

const struct SMapI *smapi_init_with(const struct SMapIParams params) {
	const struct PMapParams pmap_params = {
		.equal_key = params.case_insensitive_key ? (fn_equal)equal_strcasecmp : (fn_equal)equal_strcmp,
		.equal_val = equal_val_size_t,
		.alloc_key = clone_strdup,
		.alloc_val = clone_val_size_t,
		.free_key = (fn_free)free,
		.free_val = (fn_free)free,
		.clone_val = clone_val_size_t,
		.str_key = (fn_str)str_or_null,
		.str_val = str_val_size_t,
		.allow_null_val = false,
		.initial = params.initial,
		.grow = params.grow,
	};

	struct SMapI *map =  calloc(1, sizeof(struct SMapI));
	map->pmap = pmap_init_with(pmap_params);;
	memcpy((void*)&map->params, &params, sizeof(struct SMapIParams));

	return map;
}

const struct SMapI *smapi_clone(const struct SMapI* const from) {
	if (!from)
		return NULL;

	struct SMapI *to = calloc(1, sizeof(struct SMapI));
	to->pmap = pmap_clone_deep(from->pmap);
	memcpy((void*)&to->params, &from->params, sizeof(struct SMapIParams));

	return to;
}

void smapi_free(const struct SMapI* const map) {
	if (!map)
		return;

	pmap_free_vals(map->pmap);

	free((void*)map);
}

void smapi_it_free(const struct SMapIIt* const it) {
	if (!it)
		return;

	if (it->st) {
		free((void*)it->st->match_data);
		pmap_it_free(it->st->pit);
	}

	free(it->st);
	free((void*)it);
}

size_t smapi_get(const struct SMapI* const map, const char* const key) {
	if (!map)
		return 0;

	const size_t *val = pmap_get(map->pmap, key);

	if (val) {
		return *val;
	} else {
		return 0;
	}
}

bool smapi_getp(size_t* val, const struct SMapI* const map, const char* const key) {
	if (!map || !val)
		return false;

	const size_t *val_p = pmap_get(map->pmap, key);

	if (val_p) {
		*val = *val_p;
		return true;
	} else {
		*val = 0;
		return false;
	}
}

bool smapi_contains_key(const struct SMapI* const map, const char* const key) {
	return map ? pmap_contains_key(map->pmap, key) : false;
}

struct SMapIPair smapi_match(const struct SMapI* const map, fn_match_smapi match, const void* const data) {
	struct SMapIPair res = { 0 };

	if (!map || !match)
		return res;

	struct SMapIItMatchData match_data = {
		.match = match,
		.data = data,
	};

	struct PMapPair pres = pmap_match(map->pmap, match_key_val_wrapper, &match_data);

	res.key = pres.key;
	res.val = pres.val ? *(size_t*)pres.val : 0;

	return res;
}

const struct SMapIIt *smapi_it(const struct SMapI* const map) {
	return map ? it_init(pmap_it(map->pmap)) : NULL;
}

const struct SMapIIt *smapi_match_it(const struct SMapI* const map, fn_match_smapi match, const void* const data) {
	if (!map || !match)
		return NULL;

	struct SMapIItMatchData *match_data = calloc(1, sizeof(struct SMapIItMatchData));
	match_data->match = match;
	match_data->data = data;

	struct SMapIIt *it = it_init(pmap_match_it(map->pmap, match_key_val_wrapper, match_data));

	if (it) {
		it->st->match_data = match_data;
		return it;
	} else {
		free(match_data);
		return NULL;
	}
}

const struct SMapIIt *smapi_it_next(const struct SMapIIt* const it) {
	if (!it)
		return NULL;


	if (!it->st) {
		smapi_it_free(it);
		return NULL;
	}

	it->st->pit = pmap_it_next(it->st->pit);

	if (it->st->pit) {
		struct SMapIIt *it_m = (struct SMapIIt*)it;
		it_m->key = it->st->pit->key;
		it_m->val = *(size_t*)it->st->pit->val;
		return it;
	} else {
		smapi_it_free(it);
		return NULL;
	}
}

bool smapi_put(const struct SMapI* const map, const char* const key, const size_t val) {
	return map ? pmap_put_free(map->pmap, key, &val): false;
}

bool smapi_put_if_absent(const struct SMapI* const map, const char* const key, const size_t val) {
	return map ? pmap_put_if_absent(map->pmap, key, &val) : NULL;
}

bool smapi_remove(const struct SMapI* const map, const char* const key) {
	return map ? pmap_remove_free(map->pmap, key) : false;
}

bool smapi_equal(const struct SMapI* const a, const struct SMapI* const b) {
	return a && b ? pmap_equal(a->pmap, b->pmap) : false;
}

struct SList *smapi_keys_slist_deep(const struct SMapI* const map) {
	return map ? pmap_keys_slist_deep(map->pmap) : NULL;
}

char *smapi_str(const struct SMapI* const map) {
	return map ? pmap_str(map->pmap) : NULL;
}

size_t smapi_size(const struct SMapI* const map) {
	return map ? pmap_size(map->pmap) : 0;
}
