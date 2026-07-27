#include <stdarg.h>
#include <stddef.h>

#include "ppmap.h"
#include "pset.h"
#include "sset.h"
#include "spmap.h"
#include "ssmap.h"

#include "util-col.h"

size_t pset_add_many_v(const struct Pset* const set, va_list __args) {
	if (!set)
		return 0;

	size_t added = 0;

	const void *val;
	while ((val = va_arg(__args, void*))) {
		if (pset_add(set, val)) {
			added++;
		}
	}

	return added;
}

size_t pset_add_many(const struct Pset* const set, ...) {
	if (!set)
		return 0;

	va_list ap;
	va_start(ap, set);

	size_t added = pset_add_many_v(set, ap);

	va_end(ap);

	return added;
}

size_t plist_append_many_v(const struct Plist* const list, va_list __args) {
	if (!list)
		return 0;

	size_t added = 0;

	const void *val;
	while ((val = va_arg(__args, void*))) {
		if (plist_append(list, val)) {
			added++;
		}
	}

	return added;
}

size_t plist_append_many(const struct Plist* const list, ...) {
	if (!list)
		return 0;

	va_list ap;
	va_start(ap, list);

	size_t added = plist_append_many_v(list, ap);

	va_end(ap);

	return added;
}

struct Slist {
	const struct SlistParams params;
	const struct Plist *plist;
};

size_t slist_append_many(const struct Slist* const list, ...) {
	if (!list)
		return 0;

	va_list ap;
	va_start(ap, list);

	size_t added = plist_append_many_v(list->plist, ap);

	va_end(ap);

	return added;
}

struct Sset {
	const struct SsetParams params;
	const struct Pset *pset;
};

size_t sset_add_many(const struct Sset* const set, ...) {
	if (!set)
		return 0;

	va_list ap;
	va_start(ap, set);

	size_t added = pset_add_many_v(set->pset, ap);

	va_end(ap);

	return added;
}

size_t ppmap_put_many_v(const struct PPmap* const map, va_list __args) {
	if (!map)
		return 0;

	size_t added = 0;

	const void *key;

	// NULL terminator is odd vararg: the key
	while ((key = va_arg(__args, void*))) {

		// trust that a value has been passed, NULL is valid
		const void *val = va_arg(__args, void*);

		if (ppmap_put_free(map, key, val)) {
			added++;
		}
	}

	return added;
}

size_t ppmap_put_many(const struct PPmap* const map, ...) {
	if (!map)
		return 0;

	va_list ap;
	va_start(ap, map);

	size_t added = ppmap_put_many_v(map, ap);

	va_end(ap);

	return added;
}

struct SPmap {
	const struct SPmapParams params;
	const struct PPmap *ppmap;
};

size_t spmap_put_many(const struct SPmap* const map, ...) {
	if (!map)
		return 0;

	va_list ap;
	va_start(ap, map);

	size_t added = ppmap_put_many_v(map->ppmap, ap);

	va_end(ap);

	return added;
}

struct SSmap {
	const struct SSmapParams params;
	const struct PPmap *ppmap;
};

size_t ssmap_put_many(const struct SSmap* const map, ...) {
	if (!map)
		return 0;

	va_list ap;
	va_start(ap, map);

	size_t added = ppmap_put_many_v(map->ppmap, ap);

	va_end(ap);

	return added;
}
