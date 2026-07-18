#ifndef PPMAP_H
#define PPMAP_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * Array backed pointer indexed map.
 * Entries preserve insertion order.
 * Operations linearly traverse keys.
 * NULL values permitted.
 */
struct PPmap; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct PPmapItState; // IWYU pragma: keep
struct PPmapIt {
	const void *key;
	const void *val;
	struct PPmapItState *st;
};

/*
 * Key/Val
 */
struct PPmapPair {
	const void *key;
	const void *val;
};

/*
 * Optional constructor params (default)
 */
struct PPmapParams {
	const fn_equal equal_key;  // compare key pointers
	const fn_equal equal_val;  // compare val pointers
	const fn_clone alloc_key;  // assign key pointer
	const fn_clone alloc_val;  // assign val pointer
	const fn_free free_key;    // NOP
	const fn_free free_val;    // free
	const fn_clone clone_val;  // NOP
	const fn_str str_key;      // %p
	const fn_str str_val;      // %p
	const bool allow_null_val; // false
	const size_t initial;      // 10
	const size_t grow;         // 10
};

/*
 * Lifecycle
 */

// construct with PPmapParams defaults
const struct PPmap *ppmap_init(void);

// construct with params
const struct PPmap *ppmap_init_with(const struct PPmapParams params);

// same params, caller frees keys when alloc_key present and vals when alloc_val present [alloc_key, alloc_val]
const struct PPmap *ppmap_clone(const struct PPmap* const from);

// same params, caller frees keys when alloc_key present, caller frees vals, NULL on NULL clone_val, alloc_val overrides clone_val [alloc_key, alloc_val, clone_val]
const struct PPmap *ppmap_clone_deep(const struct PPmap* const from);

// free map
void ppmap_free(const struct PPmap* const map);

// free map and vals [free_val]
void ppmap_free_vals(const struct PPmap* const map);

// free iterator
void ppmap_it_free(const struct PPmapIt* const it);

/*
 * Access
 */

// return val, NULL if not present [equal_key]
const void *ppmap_get(const struct PPmap* const map, const void* const key);

// true if key is present [equal_key]
bool ppmap_contains_key(const struct PPmap* const map, const void* const key);

// true if val is present [equal_val]
bool ppmap_contains_val(const struct PPmap* const map, const void* const val);

// element at zero indexed position
struct PPmapPair ppmap_at(const struct PPmap* const map, const size_t i);

// find the first key/val pred, {NULL,NULL} when no matches or NULL match
struct PPmapPair ppmap_find(const struct PPmap* const map, fn_3pred pred_key_val, const void* const data);

// find the first key pred, {NULL,NULL} when no matches or NULL match
struct PPmapPair ppmap_find_key(const struct PPmap* const map, fn_2pred pred_key, const void* const data);

// find the first val pred, {NULL,NULL} when no matches or NULL match
struct PPmapPair ppmap_find_val(const struct PPmap* const map, fn_2pred pred_val, const void* const data);

// create an iterator, caller must ppmap_it_free or invoke ppmap_next until NULL
const struct PPmapIt *ppmap_it(const struct PPmap* const map);

// create an iterator filtering by key/val pred, return NULL when no matches or NULL match
const struct PPmapIt *ppmap_filter_it(const struct PPmap* const map, fn_3pred pred_key_val, const void* const data);

// create an iterator filtering by key pred, return NULL when no matches or NULL match
const struct PPmapIt *ppmap_key_filter_it(const struct PPmap* const map, fn_2pred pred_key, const void* const data);

// create an iterator filtering by val pred, return NULL when no matches or NULL match
const struct PPmapIt *ppmap_val_filter_it(const struct PPmap* const map, fn_2pred pred_val, const void* const data);

// next iterator entry, NULL at end of map
const struct PPmapIt *ppmap_it_next(const struct PPmapIt* const it);

/*
 * Mutate
 */

// set key/val, return old val if overwritten [equal_key, alloc_key, alloc_val]
const void *ppmap_put(const struct PPmap* const map, const void* const key, const void* const val);

// set key/val if not present, return existing val if present [equal_key, alloc_key, alloc_val]
const void *ppmap_put_if_absent(const struct PPmap* const map, const void* const key, const void* const val);

// set key/val, free old val, return true if overwritten [equal_key, alloc_key, alloc_val, free_key, free_val]
bool ppmap_put_free(const struct PPmap* const map, const void* const key, const void* const val);

// set all from key/val, returning number overwritten [equal_key, alloc_key, alloc_val]
size_t ppmap_put_all(const struct PPmap* const map, const struct PPmap* const from);

// set all from key/val, returning number overwritten, freeing overwritten vals [equal_key, alloc_key, alloc_val, free_val]
size_t ppmap_put_all_free(const struct PPmap* const map, const struct PPmap* const from);

// set all from key/val, returning number overwritten, NOP when NULL clone_val [equal_key, alloc_key, clone_val]
size_t ppmap_put_all_clone(const struct PPmap* const map, const struct PPmap* const from);

// set all from key/val, returning number overwritten, freeing overwritten vals, NOP when NULL clone_val [equal_key, alloc_key, free_val, clone_val]
size_t ppmap_put_all_clone_free(const struct PPmap* const map, const struct PPmap* const from);

// remove entry, if removed return old val [equal_key, free_key]
const void *ppmap_remove(const struct PPmap* const map, const void* const key);

// remove and free entry, if removed free it and return true [equal_key, free_key, free_val]
bool ppmap_remove_free(const struct PPmap* const map, const void* const key);

// remove entries matching from keys, return number removed [equal_key, free_key]
size_t ppmap_remove_all(const struct PPmap* const map, const struct PPmap* const from);

// remove and free entries matching from keys, return number removed [equal_key, free_key, free_val]
size_t ppmap_remove_all_free(const struct PPmap* const map, const struct PPmap* const from);

// remove the entry, it is unusable, ppmap_it_next must be called [free_key]
void ppmap_it_remove(const struct PPmapIt* const it);

// remove and entry, free the val, it is unusable, ppmap_it_next must be called [free_key, free_val]
void ppmap_it_remove_free(const struct PPmapIt* const it);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses params from a [equal_key, equal_val]
bool ppmap_equal(const struct PPmap* const a, const struct PPmap* const b);

/*
 * Conversion
 */

// map ordered keys, caller frees list, caller frees contents when alloc_key present [alloc_key]
struct Pslist *ppmap_keys_pslist(const struct PPmap* const map);

// map ordered keys, same params, caller frees contents when alloc_key present [alloc_key]
const struct Pset *ppmap_keys_pset(const struct PPmap* const map);

// map ordered vals, caller frees list, caller frees contents when alloc_val present [alloc_val]
struct Pslist *ppmap_vals_pslist(const struct PPmap* const map);

// map ordered vals, caller frees list and vals, NULL when NULL clone_val [clone_val]
struct Pslist *ppmap_vals_pslist_clone(const struct PPmap* const map);

// map ordered vals, same params, caller frees set, caller frees vals when alloc_val present [alloc_val]
const struct Pset *ppmap_vals_pset(const struct PPmap* const map);

// map ordered vals, same params, caller frees set and vals, NULL on NULL clone_val, alloc_val overrides clone_val [alloc_val, clone_val]
const struct Pset *ppmap_vals_pset_clone(const struct PPmap* const map);

/*
 * Info
 */

// to string, user frees, format "str_key = str_val\n"
char *ppmap_str(const struct PPmap* const map);

// number of entries
size_t ppmap_size(const struct PPmap* const map);

#endif // PPMAP_H

