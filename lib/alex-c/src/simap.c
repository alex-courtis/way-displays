#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "fn.h"
#include "ppmap.h"
#include "sset.h"

#include "simap.h"

struct SImap {
	const struct SImapParams params;
	const struct PPmap *ppmap;
};

struct SImapFilterData {
	fn_3pred_str_szt pred_key_val;
	fn_2pred_str pred_key;
	fn_2pred_szt pred_val;
	const void *data;
};

struct SImapItState {
	const struct PPmapIt *pit;
	const struct SImapFilterData *filter_data;
};

static bool pred_key_val_wrapper(const void* const key, const void* const val, const void* const data) {
	const struct SImapFilterData* const filter_data = data;
	return filter_data->pred_key_val(key, *(size_t*)val, filter_data->data);
}

static bool pred_key_wrapper(const void* const key, const void* const data) {
	const struct SImapFilterData* const filter_data = data;
	return filter_data->pred_key(key, filter_data->data);
}

static bool pred_val_wrapper(const void* const val, const void* const data) {
	const struct SImapFilterData* const filter_data = data;
	return filter_data->pred_val(*(size_t*)val, filter_data->data);
}

static struct SImapIt *it_init(const struct PPmapIt *pit) {
	if (!pit)
		return NULL;

	struct SImapIt *it = calloc(1, sizeof(struct SImapIt));
	it->st = calloc(1, sizeof(struct SImapItState));

	it->st->pit = pit;
	it->key = pit->key;
	it->val = *(size_t*)pit->val;

	return it;
}

const struct SImap *simap_init(void) {
	const struct SImapParams params = { 0 };
	return simap_init_with(params);
}

const struct SImap *simap_init_with(const struct SImapParams params) {
	const struct PPmapParams ppmap_params = {
		.equal_key = params.case_insensitive_key ? (fn_equal)equal_strcasecmp : (fn_equal)equal_strcmp,
		.equal_val = (fn_equal)equal_stp,
		.alloc_key = (fn_clone)clone_strdup,
		.alloc_val = (fn_clone)clone_size_t_ptr,
		.free_key = free,
		.free_val = free,
		.str_key = (fn_str)str_or_null,
		.str_val = (fn_str)str_size_t_ptr,
		.allow_null_val = false,
		.initial = params.initial,
		.grow = params.grow,
	};

	struct SImap *map =  calloc(1, sizeof(struct SImap));
	map->ppmap = ppmap_init_with(ppmap_params);;
	memcpy((void*)&map->params, &params, sizeof(struct SImapParams));

	return map;
}

const struct SImap *simap_clone(const struct SImap* const from) {
	if (!from)
		return NULL;

	struct SImap *to = calloc(1, sizeof(struct SImap));
	to->ppmap = ppmap_clone(from->ppmap);
	memcpy((void*)&to->params, &from->params, sizeof(struct SImapParams));

	return to;
}

void simap_free(const struct SImap* const map) {
	if (!map)
		return;

	ppmap_free_vals(map->ppmap);

	free((void*)map);
}

void simap_it_free(const struct SImapIt* const it) {
	if (!it)
		return;

	if (it->st) {
		free((void*)it->st->filter_data);
		ppmap_it_free(it->st->pit);
	}

	free(it->st);
	free((void*)it);
}

size_t simap_get(const struct SImap* const map, const char* const key) {
	if (!map)
		return 0;

	const size_t *val = ppmap_get(map->ppmap, key);

	if (val) {
		return *val;
	} else {
		return 0;
	}
}

bool simap_get_ptr(size_t* np, const struct SImap* const map, const char* const key) {
	if (!map || !np)
		return false;

	const size_t *vp = ppmap_get(map->ppmap, key);

	if (vp) {
		*np = *vp;
		return true;
	} else {
		*np = 0;
		return false;
	}
}

bool simap_contains_key(const struct SImap* const map, const char* const key) {
	return map ? ppmap_contains_key(map->ppmap, key) : false;
}

bool simap_contains_val(const struct SImap* const map, const size_t val) {
	return map ? ppmap_contains_val(map->ppmap, &val) : false;
}

struct SImapPair simap_at(const struct SImap* const map, const size_t i) {
	struct SImapPair res = { 0 };

	if (!map)
		return res;

	struct PPmapPair pres = ppmap_at(map->ppmap, i);

	res.key = pres.key;
	res.val = pres.val ? *(size_t*)pres.val : 0;

	return res;
}

struct SImapPair simap_find(const struct SImap* const map, fn_3pred_str_szt pred_key_val, const void* const data) {
	struct SImapPair res = { 0 };

	if (!map || !pred_key_val)
		return res;

	struct SImapFilterData filter_data = {
		.pred_key_val = pred_key_val,
		.data = data,
	};

	struct PPmapPair pres = ppmap_find(map->ppmap, pred_key_val_wrapper, &filter_data);

	res.key = pres.key;
	res.val = pres.val ? *(size_t*)pres.val : 0;

	return res;
}

struct SImapPair simap_find_key(const struct SImap* const map, fn_2pred_str pred_key, const void* const data) {
	struct SImapPair res = { 0 };

	if (!map || !pred_key)
		return res;

	struct SImapFilterData filter_data = {
		.pred_key = pred_key,
		.data = data,
	};

	struct PPmapPair pres = ppmap_find_key(map->ppmap, pred_key_wrapper, &filter_data);

	res.key = pres.key;
	res.val = pres.val ? *(size_t*)pres.val : 0;

	return res;
}

struct SImapPair simap_find_val(const struct SImap* const map, fn_2pred_szt pred_val, const void* const data) {
	struct SImapPair res = { 0 };

	if (!map || !pred_val)
		return res;

	struct SImapFilterData filter_data = {
		.pred_val = pred_val,
		.data = data,
	};

	struct PPmapPair pres = ppmap_find_val(map->ppmap, pred_val_wrapper, &filter_data);

	res.key = pres.key;
	res.val = pres.val ? *(size_t*)pres.val : 0;

	return res;
}

const struct SImapIt *simap_it(const struct SImap* const map) {
	return map ? it_init(ppmap_it(map->ppmap)) : NULL;
}

const struct SImapIt *simap_filter_it(const struct SImap* const map, fn_3pred_str_szt pred_key_val, const void* const data) {
	if (!map || !pred_key_val)
		return NULL;

	struct SImapFilterData *filter_data = calloc(1, sizeof(struct SImapFilterData));
	filter_data->pred_key_val = pred_key_val;
	filter_data->data = data;

	struct SImapIt *it = it_init(ppmap_filter_it(map->ppmap, pred_key_val_wrapper, filter_data));

	if (it) {
		it->st->filter_data = filter_data;
		return it;
	} else {
		free(filter_data);
		return NULL;
	}
}

const struct SImapIt *simap_key_filter_it(const struct SImap* const map, fn_2pred_str pred_key, const void* const data) {
	if (!map || !pred_key)
		return NULL;

	struct SImapFilterData *filter_data = calloc(1, sizeof(struct SImapFilterData));
	filter_data->pred_key = pred_key;
	filter_data->data = data;

	struct SImapIt *it = it_init(ppmap_key_filter_it(map->ppmap, pred_key_wrapper, filter_data));

	if (it) {
		it->st->filter_data = filter_data;
		return it;
	} else {
		free(filter_data);
		return NULL;
	}
}

const struct SImapIt *simap_val_filter_it(const struct SImap* const map, fn_2pred_szt pred_val, const void* const data) {
	if (!map || !pred_val)
		return NULL;

	struct SImapFilterData *filter_data = calloc(1, sizeof(struct SImapFilterData));
	filter_data->pred_val = pred_val;
	filter_data->data = data;

	struct SImapIt *it = it_init(ppmap_val_filter_it(map->ppmap, pred_val_wrapper, filter_data));

	if (it) {
		it->st->filter_data = filter_data;
		return it;
	} else {
		free(filter_data);
		return NULL;
	}
}

const struct SImapIt *simap_it_next(const struct SImapIt* const it) {
	if (!it)
		return NULL;


	if (!it->st) {
		simap_it_free(it);
		return NULL;
	}

	it->st->pit = ppmap_it_next(it->st->pit);

	if (it->st->pit) {
		struct SImapIt *it_m = (struct SImapIt*)it;
		it_m->key = it->st->pit->key;
		it_m->val = *(size_t*)it->st->pit->val;
		return it;
	} else {
		simap_it_free(it);
		return NULL;
	}
}

bool simap_put(const struct SImap* const map, const char* const key, const size_t val) {
	return map ? ppmap_put_free(map->ppmap, key, &val): false;
}

bool simap_put_if_absent(const struct SImap* const map, const char* const key, const size_t val) {
	return map ? ppmap_put_if_absent(map->ppmap, key, &val) : NULL;
}

size_t simap_put_all(const struct SImap* const map, const struct SImap* const from) {
	return map && from ? ppmap_put_all_free(map->ppmap, from->ppmap) : 0;
}

bool simap_remove(const struct SImap* const map, const char* const key) {
	return map ? ppmap_remove_free(map->ppmap, key) : false;
}

size_t simap_remove_all(const struct SImap* const map, const struct SImap* const from) {
	return map && from ? ppmap_remove_all_free(map->ppmap, from->ppmap) : false;
}

void simap_it_remove(const struct SImapIt* const it) {
	if (!it)
		return;

	if (!it->st) {
		simap_it_free(it);
		return;
	}

	ppmap_it_remove_free(it->st->pit);

	((struct SImapIt*)it)->key = NULL;
	((struct SImapIt*)it)->val = 0;
}

bool simap_equal(const struct SImap* const a, const struct SImap* const b) {
	return a && b ? ppmap_equal(a->ppmap, b->ppmap) : false;
}

struct Pslist *simap_keys_pslist(const struct SImap* const map) {
	return map ? ppmap_keys_pslist(map->ppmap) : NULL;
}

const struct Sset *simap_keys_sset(const struct SImap* const map) {
	if (!map)
		return NULL;

	const struct SsetParams params = {
		.case_insensitive = map->params.case_insensitive_key,
		.initial = MAX(ppmap_size(map->ppmap), map->params.initial),
		.grow = map->params.grow,
	};
	const struct Sset *set = sset_init_with(params);

	for (const struct SImapIt *it = simap_it(map); it; it = simap_it_next(it)) {
		sset_add(set, it->key);
	}

	return set;
}

char *simap_str(const struct SImap* const map) {
	return map ? ppmap_str(map->ppmap) : NULL;
}

size_t simap_size(const struct SImap* const map) {
	return map ? ppmap_size(map->ppmap) : 0;
}
