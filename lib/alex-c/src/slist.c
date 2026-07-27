#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "plist.h"

#include "slist.h"

struct Slist {
	const struct SlistParams params;
	const struct Plist *plist;
};

struct SlistItState {
	const struct PlistIt *pit;
};

static const struct SlistIt *it_init(const struct PlistIt *pit) {
	if (!pit)
		return NULL;

	struct SlistIt *it = calloc(1, sizeof(struct SlistIt));
	it->st = calloc(1, sizeof(struct SlistItState));

	it->st->pit = pit;
	it->val = pit->val;

	return it;
}

static const struct SlistIt *it_next_prev(const struct SlistIt* const it, bool prev) {
	if (!it->st) {
		slist_it_free(it);
		return NULL;
	}

	if (prev) {
		it->st->pit = plist_it_prev(it->st->pit);
	} else {
		it->st->pit = plist_it_next(it->st->pit);
	}

	if (it->st->pit) {
		struct SlistIt *it_m = (struct SlistIt*)it;
		it_m->val = it->st->pit->val;
		return it;
	} else {
		slist_it_free(it);
		return NULL;
	}
}

static struct PlistFilter plist_filter_init(const struct SlistFilter *filter) {
	const struct PlistFilter ppmap_filter = {
		.val = (fn_pred_p)filter->val,
		.data = filter->data,
		.val_data = (fn_pred_pp)filter->val_data,
	};

	return ppmap_filter;
}

const struct Slist *slist_init(void) {
	const struct SlistParams params = { 0 };
	return slist_init_with(params);
}

const struct Slist *slist_init_with(const struct SlistParams params) {
	const struct PlistParams plist_params = {
		.equal_val = params.case_insensitive ? (fn_equal)equal_strcasecmp : (fn_equal)equal_strcmp,
		.alloc_val = (fn_clone)clone_strdup,
		.free_val = free,
		.str_val = (fn_str)str_or_null,
		.allow_null_val = params.allow_null_val,
		.initial = params.initial,
		.grow = params.grow,
	};

	struct Slist *list = calloc(1, sizeof(struct Slist));
	list->plist = plist_init_with(plist_params);;
	memcpy((void*)&list->params, &params, sizeof(struct SlistParams));

	return list;
}

const struct Slist *slist_clone(const struct Slist* const from) {
	if (!from)
		return NULL;

	struct Slist *to = calloc(1, sizeof(struct Slist));
	to->plist = plist_clone(from->plist);
	memcpy((void*)&to->params, &from->params, sizeof(struct SlistParams));

	return to;
}

void slist_free(const struct Slist* const list) {
	if (!list)
		return;

	plist_free_vals(list->plist);

	free((void*)list);
}

void slist_it_free(const struct SlistIt* const it) {
	if (!it)
		return;

	if (it->st)
		plist_it_free(it->st->pit);

	free((void*)it->st);
	free((void*)it);
}

bool slist_contains(const struct Slist* const list, const char* const val) {
	return list ? plist_contains(list->plist, val) : false;
}

bool slist_index_of(size_t *index, const struct Slist* const list, const char* const val) {
	if (index)
		*index = 0;

	if (!list)
		return false;

	return plist_index_of(index, list->plist, val);
}

const char *slist_at(const struct Slist* const list, const size_t i) {
	return list ? plist_at(list->plist, i) : NULL;
}

const char *slist_find(const struct Slist* const list, const struct SlistFilter filter) {
	return list ? plist_find(list->plist, plist_filter_init(&filter)) : NULL;
}

const struct SlistIt *slist_it_start(const struct Slist* const list) {
	return list ? it_init(plist_it_start(list->plist)) : NULL;
}

const struct SlistIt *slist_it_end(const struct Slist* const list) {
	return list ? it_init(plist_it_end(list->plist)) : NULL;
}

const struct SlistIt *slist_filter_it_start(const struct Slist* const list, const struct SlistFilter filter) {
	return list ? it_init(plist_filter_it_start(list->plist, plist_filter_init(&filter))) : NULL;
}

const struct SlistIt *slist_filter_it_end(const struct Slist* const list, const struct SlistFilter filter) {
	return list ? it_init(plist_filter_it_end(list->plist, plist_filter_init(&filter))) : NULL;
}

const struct SlistIt *slist_it_next(const struct SlistIt* const it) {
	return it ? it_next_prev(it, false) : NULL;
}

const struct SlistIt *slist_it_prev(const struct SlistIt* const it) {
	return it ? it_next_prev(it, true) : NULL;
}

bool slist_insert(const struct Slist* const list, size_t index, const char* const val) {
	return list ? plist_insert(list->plist, index, val) : false;
}

bool slist_append(const struct Slist* const list, const char* const val) {
	return list ? plist_append(list->plist, val) : false;
}

bool slist_prepend(const struct Slist* const list, const char* const val) {
	return list ? plist_prepend(list->plist, val) : false;
}

bool slist_replace(const struct Slist* const list, size_t index, const char* const val) {
	return list ? plist_replace_free(list->plist, index, val) : false;
}

size_t slist_append_all(const struct Slist* const list, const struct Slist* const from) {
	return list && from ? plist_append_all(list->plist, from->plist) : 0;
}

bool slist_remove(const struct Slist* const list, const char* const val) {
	return list ? plist_remove_free(list->plist, val) : false;
}

bool slist_remove_at(const struct Slist* const list, const size_t i) {
	return list ? plist_remove_at_free(list->plist, i) : false;
}

size_t slist_remove_all(const struct Slist* const list) {
	return list ? plist_remove_all_free(list->plist) : 0;
}

bool slist_it_remove(const struct SlistIt* const it) {
	if (!it)
		return false;

	if (!it->st) {
		slist_it_free(it);
		return false;
	}

	((struct SlistIt*)it)->val = NULL;

	return plist_it_remove_free(it->st->pit);
}

void slist_sort(const struct Slist* const list) {
	if (list)
		plist_sort(list->plist, list->params.case_insensitive ? (fn_pred_pp)less_than_strcasecmp : (fn_pred_pp)less_than_strcmp);
}

bool slist_equal(const struct Slist* const a, const struct Slist* const b) {
	return a && b ? plist_equal(a->plist, b->plist) : false;
}

bool slist_equal_ordered(const struct Slist* const a, const struct Slist* const b) {
	return a && b ? plist_equal_ordered(a->plist, b->plist) : false;
}

char *slist_str(const struct Slist* const list) {
	return list ? plist_str(list->plist) : NULL;
}

size_t slist_size(const struct Slist* const list) {
	return list ? plist_size(list->plist) : 0;
}
