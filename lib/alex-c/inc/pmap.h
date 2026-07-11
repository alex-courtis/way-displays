#ifndef PMAP_H
#define PMAP_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

// TODO this is a PmapP
// PmapPit
// PmapP_It
//
// PmapP_ItState
// PmapP_ItState
//
// PmapPpair
// PmapP_Pair
//
// PmapPparams
// PmapP_Params

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

// construct with PMapParams defaults
const struct PMap *pmap_init(void);

// construct with params
const struct PMap *pmap_init_with(const struct PMapParams params);

// same params, caller frees keys when alloc_key present and vals when alloc_val present [alloc_key, alloc_val]
const struct PMap *pmap_clone(const struct PMap* const from);

// same params, caller frees keys when alloc_key present, caller frees vals, NULL on NULL clone_val, alloc_val overrides clone_val [alloc_key, alloc_val, clone_val]
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
struct PMapPair pmap_match(const struct PMap* const map, fn_3pred match, const void* const data);

// find the first key match, {NULL,NULL} when no matches or NULL match
struct PMapPair pmap_match_key(const struct PMap* const map, fn_2pred match, const void* const data);

// find the first val match, {NULL,NULL} when no matches or NULL match
struct PMapPair pmap_match_val(const struct PMap* const map, fn_2pred match, const void* const data);

// create an iterator, caller must pmap_it_free or invoke pmap_next until NULL
const struct PMapIt *pmap_it(const struct PMap* const map);

// create an iterator filtering by key/val match, return NULL when no matches or NULL match
const struct PMapIt *pmap_match_it(const struct PMap* const map, fn_3pred match, const void* const data);

// create an iterator filtering by key match, return NULL when no matches or NULL match
const struct PMapIt *pmap_match_key_it(const struct PMap* const map, fn_2pred match, const void* const data);

// create an iterator filtering by val match, return NULL when no matches or NULL match
const struct PMapIt *pmap_match_val_it(const struct PMap* const map, fn_2pred match, const void* const data);

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

// set all from key/val, returning number overwritten [equal_key, alloc_key, alloc_val]
size_t pmap_put_all(const struct PMap* const map, const struct PMap* const from);

// set all from key/val, returning number overwritten, freeing overwritten vals [equal_key, alloc_key, alloc_val, free_val]
size_t pmap_put_all_free(const struct PMap* const map, const struct PMap* const from);

// set all from key/val, returning number overwritten, NOP when NULL clone_val [equal_key, alloc_key, clone_val]
size_t pmap_put_all_clone(const struct PMap* const map, const struct PMap* const from);

// set all from key/val, returning number overwritten, freeing overwritten vals, NOP when NULL clone_val [equal_key, alloc_key, free_val, clone_val]
size_t pmap_put_all_clone_free(const struct PMap* const map, const struct PMap* const from);

// remove entry, if removed return old val [equal_key, free_key]
const void *pmap_remove(const struct PMap* const map, const void* const key);

// remove and free entry, if removed free it and return true [equal_key, free_key, free_val]
bool pmap_remove_free(const struct PMap* const map, const void* const key);

// remove entries matching from keys, return number removed [equal_key, free_key]
size_t pmap_remove_all(const struct PMap* const map, const struct PMap* const from);

// remove and free entries matching from keys, return number removed [equal_key, free_key, free_val]
size_t pmap_remove_all_free(const struct PMap* const map, const struct PMap* const from);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses params from a [equal_key, equal_val]
bool pmap_equal(const struct PMap* const a, const struct PMap* const b);

/*
 * Conversion
 */

// map ordered keys, caller frees list, caller frees contents when alloc_key present [alloc_key]
struct Pslist *pmap_keys_pslist(const struct PMap* const map);

// map ordered keys, same params, caller frees contents when alloc_key present [alloc_key]
const struct PSet *pmap_keys_pset(const struct PMap* const map);

// map ordered vals, caller frees list, caller frees contents when alloc_val present [alloc_val]
struct Pslist *pmap_vals_pslist(const struct PMap* const map);

// map ordered vals, caller frees list and vals, NULL when NULL clone_val [clone_val]
struct Pslist *pmap_vals_pslist_clone(const struct PMap* const map);

// map ordered vals, same params, caller frees set, caller frees vals when alloc_val present [alloc_val]
const struct PSet *pmap_vals_pset(const struct PMap* const map);

// map ordered vals, same params, caller frees set and vals, NULL on NULL clone_val, alloc_val overrides clone_val [alloc_val, clone_val]
const struct PSet *pmap_vals_pset_clone(const struct PMap* const map);

/*
 * Info
 */

// to string, user frees, format "str_key = str_val\n"
char *pmap_str(const struct PMap* const map);

// number of entries
size_t pmap_size(const struct PMap* const map);

#endif // PMAP_H

