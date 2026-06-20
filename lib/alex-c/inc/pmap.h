#ifndef PMAP_H
#define PMAP_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * Array backed pointer indexed map.
 * Entries preserve insertion order.
 * Operations linearly traverse keys.
 * NULL values permitted.
 */
struct PMap; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct PMapIterState; // IWYU pragma: keep
struct PMapIter {
	const void *key;
	const void *val;
	struct PMapIterState *st;
};

/*
 * Optional constructor params (default)
 */
struct PMapParams {
	const fn_equal equal_key; // compare key pointers
	const fn_equal equal_val; // compare val pointers
	const fn_clone clone_key; // assign key pointer
	const fn_clone clone_val; // assign val pointer
	const fn_free free_key;   // NOP
	const fn_free free_val;   // free
	const fn_str str_key;     // %p
	const fn_str str_val;     // %p
	const size_t initial;     // 10
	const size_t grow;        // 10
};

/*
 * Lifecycle
 */

// construct with PMapParams defaults
const struct PMap *pmap_init(void);

// construct with params
const struct PMap *pmap_init_with(const struct PMapParams params);

// clone, setting val pointers [clone_key]
const struct PMap *pmap_clone_shallow(const struct PMap* const from);

// clone, NOP when NULL clone_val [clone_key, clone_val]
const struct PMap *pmap_clone_deep(const struct PMap* const from);

// free map
void pmap_free(const struct PMap* const tab);

// free map and vals [free_val]
void pmap_free_vals(const struct PMap* const tab);

// free iter
void pmap_iter_free(const struct PMapIter* const iter);

/*
 * Access
 */

// return val, NULL if not present [equal_key]
const void *pmap_get(const struct PMap* const tab, const void* const key);

// true if key is present [equal_key]
bool pmap_contains_key(const struct PMap* const tab, const void* const key);

// create an iterator, caller must pmap_iter_free or invoke pmap_next until NULL
const struct PMapIter *pmap_iter(const struct PMap* const tab);

// create an iterator filtering by equal_key and equal_val, NULL tests match all
const struct PMapIter *pmap_filter_iter(const struct PMap* const tab, fn_equal equal_key, fn_equal equal_val, const void* const data);

// next iterator entry, NULL at end of map
const struct PMapIter *pmap_iter_next(const struct PMapIter* const iter);

/*
 * Mutate
 */

// set key/val, return old val if overwritten [equal_key, clone_key, clone_val]
const void *pmap_put(const struct PMap* const tab, const void* const key, const void* const val);

// set key/val if not present, return existing val if present [equal_key, clone_key, clone_val]
const void *pmap_put_if_absent(const struct PMap* const tab, const void* const key, const void* const val);

// set key/val, free old val, return true if overwritten [equal_key, clone_key, clone_val, free_key, free_val]
bool pmap_put_free(const struct PMap* const tab, const void* const key, const void* const val);

// remove val, return old val if present [equal_key, free_key]
const void *pmap_remove(const struct PMap* const tab, const void* const key);

// remove val, if removed free val and return true [equal_key, free_key, free_val]
bool pmap_remove_free(const struct PMap* const tab, const void* const key);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses params from a [equal_key, equal_val]
bool pmap_equal(const struct PMap* const a, const struct PMap* const b);

/*
 * Conversion
 */

// ordered keys, caller frees list only
struct SList *pmap_keys_slist_shallow(const struct PMap* const tab);

// ordered keys, caller frees list list and vals, NOP when NULL clone_key [clone_key]
struct SList *pmap_keys_slist_deep(const struct PMap* const tab);

// ordered vals, caller frees list only
struct SList *pmap_vals_slist_shallow(const struct PMap* const tab);

// ordered vals, caller frees list and vals, NOP when NULL clone_val [clone_val]
struct SList *pmap_vals_slist_deep(const struct PMap* const tab);

/*
 * Info
 */

// to string, user frees, format "str_key = str_val\n"
char *pmap_str(const struct PMap* const tab);

// number of entries
size_t pmap_size(const struct PMap* const tab);

#endif // PMAP_H

