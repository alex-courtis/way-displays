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
struct PMapItState; // IWYU pragma: keep
struct PMapIt {
	const void *key;
	const void *val;
	struct PMapItState *st;
};

/*
 * Key/Val
 */
struct PMapPair {
	const void *key;
	const void *val;
};

/*
 * Optional constructor params (default)
 */
struct PMapParams {
	const fn_equal equal_key;  // compare key pointers
	const fn_equal equal_val;  // compare val pointers
	const fn_alloc alloc_key;  // assign key pointer
	const fn_alloc alloc_val;  // assign val pointer
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

// construct with PMapParams defaults
const struct PMap *pmap_init(void);

// construct with params
const struct PMap *pmap_init_with(const struct PMapParams params);

// clone, setting val pointers [alloc_key]
const struct PMap *pmap_clone_shallow(const struct PMap* const from);

// clone, empty when NULL clone_val [alloc_key, clone_val]
const struct PMap *pmap_clone_deep(const struct PMap* const from);

// free map
void pmap_free(const struct PMap* const map);

// free map and vals [free_val]
void pmap_free_vals(const struct PMap* const map);

// free iterator
void pmap_it_free(const struct PMapIt* const it);

/*
 * Access
 */

// return val, NULL if not present [equal_key]
const void *pmap_get(const struct PMap* const map, const void* const key);

// true if key is present [equal_key]
bool pmap_contains_key(const struct PMap* const map, const void* const key);

// true if val is present [equal_val]
bool pmap_contains_val(const struct PMap* const map, const void* const val);

// find the first key/val match, {NULL,NULL} when no matches or NULL match
struct PMapPair pmap_match(const struct PMap* const map, fn_match_ptr_ptr match, const void* const data);

// find the first key match, {NULL,NULL} when no matches or NULL match
struct PMapPair pmap_match_key(const struct PMap* const map, fn_match_ptr match, const void* const data);

// find the first val match, {NULL,NULL} when no matches or NULL match
struct PMapPair pmap_match_val(const struct PMap* const map, fn_match_ptr match, const void* const data);

// create an iterator, caller must pmap_it_free or invoke pmap_next until NULL
const struct PMapIt *pmap_it(const struct PMap* const map);

// create an iterator filtering by key/val match, return NULL when no matches or NULL match
const struct PMapIt *pmap_match_it(const struct PMap* const map, fn_match_ptr_ptr match, const void* const data);

// create an iterator filtering by val match, return NULL when no matches or NULL match
const struct PMapIt *pmap_match_val_it(const struct PMap* const map, fn_match_ptr match, const void* const data);

// next iterator entry, NULL at end of map
const struct PMapIt *pmap_it_next(const struct PMapIt* const it);

/*
 * Mutate
 */

// set key/val, return old val if overwritten [equal_key, alloc_key, alloc_val]
const void *pmap_put(const struct PMap* const map, const void* const key, const void* const val);

// set key/val if not present, return existing val if present [equal_key, alloc_key, alloc_val]
const void *pmap_put_if_absent(const struct PMap* const map, const void* const key, const void* const val);

// set key/val, free old val, return true if overwritten [equal_key, alloc_key, alloc_val, free_key, free_val]
bool pmap_put_free(const struct PMap* const map, const void* const key, const void* const val);

// remove val, return old val if present [equal_key, free_key]
const void *pmap_remove(const struct PMap* const map, const void* const key);

// remove val, if removed free val and return true [equal_key, free_key, free_val]
bool pmap_remove_free(const struct PMap* const map, const void* const key);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses params from a [equal_key, equal_val]
bool pmap_equal(const struct PMap* const a, const struct PMap* const b);

/*
 * Conversion
 */

// map ordered keys, caller frees list only
struct SList *pmap_keys_slist_shallow(const struct PMap* const map);

// map ordered keys, caller frees list list and vals, empty when NULL alloc_key [alloc_key]
struct SList *pmap_keys_slist_deep(const struct PMap* const map);

// map ordered keys, same parameters, shallow when alloc_key is NULL
const struct PSet *pmap_keys_pset(const struct PMap* const map);

// map ordered vals, caller frees list only
struct SList *pmap_vals_slist_shallow(const struct PMap* const map);

// map ordered vals, caller frees list and vals, empty when NULL clone_val [clone_val]
struct SList *pmap_vals_slist_deep(const struct PMap* const map);

// map ordered vals, same parameters, shallow when alloc_val is NULL
const struct PSet *pmap_vals_pset(const struct PMap* const map);

/*
 * Info
 */

// to string, user frees, format "str_key = str_val\n"
char *pmap_str(const struct PMap* const map);

// number of entries
size_t pmap_size(const struct PMap* const map);

#endif // PMAP_H

