#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "fn.h"
#include "ppmap.h"
#include "slist.h"
#include "sset.h"

#include "ssmap.h"

struct SSmap {
	const struct SSmapParams params;
	const struct PPmap *ppmap;
};

struct SSmapItState {
	const struct PPmapIt *pit;
};

static struct PPmapFilter ppmap_filter_init(const struct SSmapFilter *filter) {
	const struct PPmapFilter ppmap_filter = {
		.key = (fn_pred_p)filter->key,
		.val = (fn_pred_p)filter->val,
		.key_val = (fn_pred_pp)filter->key_val,
		.data = filter->data,
		.key_data = (fn_pred_pp)filter->key_data,
		.val_data = (fn_pred_pp)filter->val_data,
		.key_val_data = (fn_pred_ppp)filter->key_val_data,
	};

	return ppmap_filter;
}

static const struct SSmapIt *it_init(const struct PPmapIt *pit) {
	if (!pit)
		return NULL;

	struct SSmapIt *it = calloc(1, sizeof(struct SSmapIt));
	it->st = calloc(1, sizeof(struct SSmapItState));

	it->st->pit = pit;
	it->key = pit->key;
	it->val = pit->val;

	return it;
}

const struct SSmap *ssmap_init(void) {
	const struct SSmapParams params = { 0 };
	return ssmap_init_with(params);
}

const struct SSmap *ssmap_init_with(const struct SSmapParams params) {
	const struct PPmapParams ppmap_params = {
		.equal_key = params.case_insensitive_key ? (fn_equal)equal_strcasecmp : (fn_equal)equal_strcmp,
		.equal_val = params.case_insensitive_val ? (fn_equal)equal_strcasecmp : (fn_equal)equal_strcmp,
		.alloc_key = (fn_clone)clone_strdup,
		.alloc_val = (fn_clone)clone_strdup,
		.free_key = free,
		.free_val = free,
		.str_key = (fn_str)str_or_null,
		.str_val = (fn_str)str_or_null,
		.initial = params.initial,
		.allow_null_val = params.allow_null_val,
		.grow = params.grow,
	};

	struct SSmap *map = calloc(1, sizeof(struct SSmap));
	map->ppmap = ppmap_init_with(ppmap_params);;
	memcpy((void*)&map->params, &params, sizeof(struct SSmapParams));

	return map;
}

const struct SSmap *ssmap_clone(const struct SSmap* const from) {
	if (!from)
		return NULL;

	struct SSmap *to = calloc(1, sizeof(struct SSmap));
	to->ppmap = ppmap_clone(from->ppmap);
	memcpy((void*)&to->params, &from->params, sizeof(struct SSmapParams));

	return to;
}

void ssmap_free(const struct SSmap* const map) {
	if (!map)
		return;

	ppmap_free_vals(map->ppmap);

	free((void*)map);
}

void ssmap_it_free(const struct SSmapIt* const it) {
	if (!it)
		return;

	if (it->st) {
		ppmap_it_free(it->st->pit);
	}

	free(it->st);
	free((void*)it);
}

const char *ssmap_get(const struct SSmap* const map, const char* const key) {
	return map ? ppmap_get(map->ppmap, key) : NULL;
}

bool ssmap_contains_key(const struct SSmap* const map, const char* const key) {
	return map ? ppmap_contains_key(map->ppmap, key) : false;
}

bool ssmap_contains_val(const struct SSmap* const map, const char* const val) {
	return map ? ppmap_contains_val(map->ppmap, val) : false;
}

const char *ssmap_first_key(const struct SSmap *const map, const char* const val) {
	return map ? ppmap_first_key(map->ppmap, val) : false;
}

struct SSmapPair ssmap_at(const struct SSmap* const map, const size_t i) {
	struct SSmapPair res = { 0 };

	if (!map)
		return res;

	struct PPmapPair pres = ppmap_at(map->ppmap, i);

	res.key = pres.key;
	res.val = pres.val;

	return res;
}

struct SSmapPair ssmap_find(const struct SSmap* const map, const struct SSmapFilter filter) {
	struct SSmapPair res = { 0 };

	if (!map)
		return res;

	struct PPmapPair pres = ppmap_find(map->ppmap, ppmap_filter_init(&filter));

	res.key = pres.key;
	res.val = pres.val;

	return res;
}

const struct SSmapIt *ssmap_it(const struct SSmap* const map) {
	return map ? it_init(ppmap_it(map->ppmap)) : NULL;
}

const struct SSmapIt *ssmap_filter_it(const struct SSmap* const map, const struct SSmapFilter filter) {
	return map ? it_init(ppmap_filter_it(map->ppmap, ppmap_filter_init(&filter))) : NULL;
}

const struct SSmapIt *ssmap_it_next(const struct SSmapIt* const it) {
	if (!it)
		return NULL;


	if (!it->st) {
		ssmap_it_free(it);
		return NULL;
	}

	it->st->pit = ppmap_it_next(it->st->pit);

	if (it->st->pit) {
		struct SSmapIt *it_m = (struct SSmapIt*)it;
		it_m->key = it->st->pit->key;
		it_m->val = it->st->pit->val;
		return it;
	} else {
		ssmap_it_free(it);
		return NULL;
	}
}

bool ssmap_put(const struct SSmap* const map, const char* const key, const char* const val) {
	return map ? ppmap_put_free(map->ppmap, key, val): false;
}

bool ssmap_put_if_absent(const struct SSmap* const map, const char* const key, const char* const val) {
	return map ? ppmap_put_if_absent(map->ppmap, key, val) : false;
}

size_t ssmap_put_all(const struct SSmap* const map, const struct SSmap* const from) {
	return map && from ? ppmap_put_all_free(map->ppmap, from->ppmap) : 0;
}

bool ssmap_remove(const struct SSmap* const map, const char* const key) {
	return map ? ppmap_remove_free(map->ppmap, key) : false;
}

size_t ssmap_remove_all(const struct SSmap* const map) {
	return map ? ppmap_remove_all_free(map->ppmap) : false;
}

size_t ssmap_remove_in(const struct SSmap* const map, const struct SSmap* const in) {
	return map && in ? ppmap_remove_in_free(map->ppmap, in->ppmap) : false;
}

bool ssmap_it_remove(const struct SSmapIt* const it) {
	if (!it)
		return false;

	if (!it->st) {
		ssmap_it_free(it);
		return false;
	}

	((struct SSmapIt*)it)->key = NULL;
	((struct SSmapIt*)it)->val = NULL;

	return ppmap_it_remove_free(it->st->pit);
}

bool ssmap_equal(const struct SSmap* const a, const struct SSmap* const b) {
	return a && b ? ppmap_equal(a->ppmap, b->ppmap) : false;
}

bool ssmap_equal_ordered(const struct SSmap* const a, const struct SSmap* const b) {
	return a && b ? ppmap_equal_ordered(a->ppmap, b->ppmap) : false;
}

const struct Slist *ssmap_keys_slist(const struct SSmap* const map) {
	if (!map)
		return NULL;

	const struct SlistParams params = {
		.case_insensitive = map->params.case_insensitive_key,
		.initial = MAX(ppmap_size(map->ppmap), map->params.initial),
		.grow = map->params.grow,
	};
	const struct Slist *list = slist_init_with(params);

	for (const struct SSmapIt *it = ssmap_it(map); it; it = ssmap_it_next(it)) {
		slist_append(list, it->key);
	}

	return list;
}

const struct Sset *ssmap_keys_sset(const struct SSmap* const map) {
	if (!map)
		return NULL;

	const struct SsetParams params = {
		.case_insensitive = map->params.case_insensitive_key,
		.initial = MAX(ppmap_size(map->ppmap), map->params.initial),
		.grow = map->params.grow,
	};
	const struct Sset *set = sset_init_with(params);

	for (const struct SSmapIt *it = ssmap_it(map); it; it = ssmap_it_next(it)) {
		sset_add(set, it->key);
	}

	return set;
}

const struct Slist *ssmap_vals_slist(const struct SSmap* const map) {
	if (!map)
		return NULL;

	const struct SlistParams params = {
		.case_insensitive = map->params.case_insensitive_val,
		.allow_null_val = map->params.allow_null_val,
		.initial = MAX(ppmap_size(map->ppmap), map->params.initial),
		.grow = map->params.grow,
	};
	const struct Slist *list = slist_init_with(params);

	for (const struct SSmapIt *it = ssmap_it(map); it; it = ssmap_it_next(it)) {
		slist_append(list, it->val);
	}

	return list;
}

char *ssmap_str(const struct SSmap* const map) {
	return map ? ppmap_str(map->ppmap) : NULL;
}

size_t ssmap_size(const struct SSmap* const map) {
	return map ? ppmap_size(map->ppmap) : 0;
}
