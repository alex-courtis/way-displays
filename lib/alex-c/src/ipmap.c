#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "ppmap.h"

#include "ipmap.h"

struct IPmap {
	const struct IPmapParams params;
	const struct PPmap *ppmap;
};

struct IPmapMatchData {
	fn_3pred_szt_ptr match_key_val;
	fn_2pred_szt match_key;
	fn_2pred match_val;
	const void *data;
};

struct IPmapItState {
	const struct PPmapIt *pit;
	const struct IPmapMatchData *match_data;
};

static bool match_key_val_wrapper(const void* const key, const void* const val, const void* const data) {
	const struct IPmapMatchData* const matcher = data;
	return matcher->match_key_val(*(size_t*)key, val, matcher->data);
}

static bool match_key_wrapper(const void* const val, const void* const data) {
	const struct IPmapMatchData* const matcher = data;
	return matcher->match_key(*(size_t*)val, matcher->data);
}

static bool match_val_wrapper(const void* const val, const void* const data) {
	const struct IPmapMatchData* const matcher = data;
	return matcher->match_val(val, matcher->data);
}

static const struct IPmap *clone(const struct IPmap* const from, bool deep) {
	if (!from)
		return NULL;

	struct IPmap *to = calloc(1, sizeof(struct IPmap));

	to->ppmap = deep ? ppmap_clone_deep(from->ppmap) : ppmap_clone(from->ppmap);

	memcpy((void*)&to->params, &from->params, sizeof(struct IPmapParams));

	return to;
}

static struct IPmapIt *it_init(const struct PPmapIt *pit) {
	if (!pit)
		return NULL;

	struct IPmapIt *it = calloc(1, sizeof(struct IPmapIt));
	it->st = calloc(1, sizeof(struct IPmapItState));

	it->st->pit = pit;
	it->key = *(size_t*)pit->key;
	it->val = pit->val;

	return it;
}

const struct IPmap *ipmap_init(void) {
	const struct IPmapParams params = { 0 };
	return ipmap_init_with(params);
}

const struct IPmap *ipmap_init_with(const struct IPmapParams params) {
	const struct PPmapParams ppmap_params = {
		.equal_key = (fn_equal)equal_stp,
		.equal_val = params.equal_val,
		.alloc_key = (fn_clone)clone_size_t_ptr,
		.alloc_val = params.alloc_val,
		.free_key = (fn_free)free,
		.free_val = params.free_val,
		.clone_val = params.clone_val,
		.str_key = (fn_str)str_size_t_ptr,
		.str_val = params.str_val,
		.allow_null_val = params.allow_null_val,
		.initial = params.initial,
		.grow = params.grow,
	};

	struct IPmap *map =  calloc(1, sizeof(struct IPmap));
	map->ppmap = ppmap_init_with(ppmap_params);;
	memcpy((void*)&map->params, &params, sizeof(struct IPmapParams));

	return map;
}

const struct IPmap *ipmap_clone(const struct IPmap* const from) {
	return clone(from, false);
}

const struct IPmap *ipmap_clone_deep(const struct IPmap* const from) {
	return clone(from, true);
}

void ipmap_free(const struct IPmap* const map) {
	if (!map)
		return;

	ppmap_free(map->ppmap);

	free((void*)map);
}

void ipmap_free_vals(const struct IPmap* const map) {
	if (!map)
		return;

	ppmap_free_vals(map->ppmap);

	free((void*)map);
}

void ipmap_it_free(const struct IPmapIt* const it) {
	if (!it)
		return;

	if (it->st) {
		free((void*)it->st->match_data);
		ppmap_it_free(it->st->pit);
	}

	free(it->st);
	free((void*)it);
}

const void *ipmap_get(const struct IPmap* const map, const size_t key) {
	return map ? ppmap_get(map->ppmap, &key) : NULL;
}

bool ipmap_contains_key(const struct IPmap* const map, const size_t key) {
	return map ? ppmap_contains_key(map->ppmap, &key) : false;
}

bool ipmap_contains_val(const struct IPmap* const map, const void* const val) {
	return map ? ppmap_contains_val(map->ppmap, val) : false;
}

struct IPmapPair ipmap_match(const struct IPmap* const map, fn_3pred_szt_ptr match, const void* const data) {
	struct IPmapPair res = { 0 };

	if (!map || !match)
		return res;

	struct IPmapMatchData match_data = {
		.match_key_val = match,
		.data = data,
	};

	struct PPmapPair pres = ppmap_match(map->ppmap, match_key_val_wrapper, &match_data);

	res.key = pres.key ? *(size_t*)pres.key : 0;
	res.val = pres.val;

	return res;
}

struct IPmapPair ipmap_match_key(const struct IPmap* const map, fn_2pred_szt match, const void* const data) {
	struct IPmapPair res = { 0 };

	if (!map || !match)
		return res;

	struct IPmapMatchData match_data = {
		.match_key = match,
		.data = data,
	};

	struct PPmapPair pres = ppmap_match_key(map->ppmap, match_key_wrapper, &match_data);

	res.key = pres.key ? *(size_t*)pres.key : 0;
	res.val = pres.val;

	return res;
}

struct IPmapPair ipmap_match_val(const struct IPmap* const map, fn_2pred match, const void* const data) {
	struct IPmapPair res = { 0 };

	if (!map || !match)
		return res;

	struct IPmapMatchData match_data = {
		.match_val = match,
		.data = data,
	};

	struct PPmapPair pres = ppmap_match_val(map->ppmap, match_val_wrapper, &match_data);

	res.key = pres.key ? *(size_t*)pres.key : 0;
	res.val = pres.val;

	return res;
}

const struct IPmapIt *ipmap_it(const struct IPmap* const map) {
	return map ? it_init(ppmap_it(map->ppmap)) : NULL;
}

const struct IPmapIt *ipmap_match_it(const struct IPmap* const map, fn_3pred_szt_ptr match, const void* const data) {
	if (!map || !match)
		return NULL;

	struct IPmapMatchData *match_data = calloc(1, sizeof(struct IPmapMatchData));
	match_data->match_key_val = match;
	match_data->data = data;

	struct IPmapIt *it = it_init(ppmap_match_it(map->ppmap, match_key_val_wrapper, match_data));

	if (it) {
		it->st->match_data = match_data;
		return it;
	} else {
		free(match_data);
		return NULL;
	}
}

const struct IPmapIt *ipmap_match_key_it(const struct IPmap* const map, fn_2pred_szt match, const void* const data) {
	if (!map || !match)
		return NULL;

	struct IPmapMatchData *match_data = calloc(1, sizeof(struct IPmapMatchData));
	match_data->match_key = match;
	match_data->data = data;

	struct IPmapIt *it = it_init(ppmap_match_key_it(map->ppmap, match_key_wrapper, match_data));

	if (it) {
		it->st->match_data = match_data;
		return it;
	} else {
		free(match_data);
		return NULL;
	}
}

const struct IPmapIt *ipmap_match_val_it(const struct IPmap* const map, fn_2pred match, const void* const data) {
	if (!map || !match)
		return NULL;

	struct IPmapMatchData *match_data = calloc(1, sizeof(struct IPmapMatchData));
	match_data->match_val = match;
	match_data->data = data;

	struct IPmapIt *it = it_init(ppmap_match_val_it(map->ppmap, match_val_wrapper, match_data));

	if (it) {
		it->st->match_data = match_data;
		return it;
	} else {
		free(match_data);
		return NULL;
	}
}

const struct IPmapIt *ipmap_it_next(const struct IPmapIt* const it) {
	if (!it)
		return NULL;


	if (!it->st) {
		ipmap_it_free(it);
		return NULL;
	}

	it->st->pit = ppmap_it_next(it->st->pit);

	if (it->st->pit) {
		struct IPmapIt *it_m = (struct IPmapIt*)it;
		it_m->key = *(size_t*)it->st->pit->key;
		it_m->val = it->st->pit->val;
		return it;
	} else {
		ipmap_it_free(it);
		return NULL;
	}
}

const void *ipmap_put(const struct IPmap* const map, const size_t key, const void* const val) {
	return map ? ppmap_put(map->ppmap, &key, val) : NULL;
}

const void *ipmap_put_if_absent(const struct IPmap* const map, const size_t key, const void* const val) {
	return map ? ppmap_put_if_absent(map->ppmap, &key, val) : NULL;
}

bool ipmap_put_free(const struct IPmap* const map, const size_t key, const char* const val) {
	return map ? ppmap_put_free(map->ppmap, &key, val) : false;
}

const void *ipmap_remove(const struct IPmap* const map, const size_t key) {
	return map ? ppmap_remove(map->ppmap, &key) : NULL;
}

bool ipmap_remove_free(const struct IPmap* const map, const size_t key) {
	return map ? ppmap_remove_free(map->ppmap, &key) : false;
}

size_t ipmap_remove_all(const struct IPmap* const map, const struct IPmap* const from) {
	return map && from ? ppmap_remove_all(map->ppmap, from->ppmap) : 0;
}

size_t ipmap_remove_all_free(const struct IPmap* const map, const struct IPmap* const from) {
	return map && from ? ppmap_remove_all_free(map->ppmap, from->ppmap) : 0;
}

size_t ipmap_put_all(const struct IPmap* const map, const struct IPmap* const from) {
	return map && from ? ppmap_put_all(map->ppmap, from->ppmap) : 0;
}

size_t ipmap_put_all_free(const struct IPmap* const map, const struct IPmap* const from) {
	return map && from ? ppmap_put_all_free(map->ppmap, from->ppmap) : 0;
}

size_t ipmap_put_all_clone(const struct IPmap* const map, const struct IPmap* const from) {
	return map && from ? ppmap_put_all_clone(map->ppmap, from->ppmap) : 0;
}

size_t ipmap_put_all_clone_free(const struct IPmap* const map, const struct IPmap* const from) {
	return map && from ? ppmap_put_all_clone_free(map->ppmap, from->ppmap) : 0;
}

bool ipmap_equal(const struct IPmap* const a, const struct IPmap* const b) {
	return a && b ? ppmap_equal(a->ppmap, b->ppmap) : false;
}

struct Pslist *ipmap_vals_pslist(const struct IPmap* const map) {
	return map ? ppmap_vals_pslist(map->ppmap) : NULL;
}

struct Pslist *ipmap_vals_pslist_clone(const struct IPmap* const map) {
	return map ? ppmap_vals_pslist_clone(map->ppmap) : NULL;
}

const struct Pset *ipmap_vals_pset(const struct IPmap* const map) {
	return map ? ppmap_vals_pset(map->ppmap) : NULL;
}

const struct Pset *ipmap_vals_pset_clone(const struct IPmap* const map) {
	return map ? ppmap_vals_pset_clone(map->ppmap) : NULL;
}

char *ipmap_str(const struct IPmap* const map) {
	return map ? ppmap_str(map->ppmap) : NULL;
}

size_t ipmap_size(const struct IPmap* const map) {
	return map ? ppmap_size(map->ppmap) : 0;
}
