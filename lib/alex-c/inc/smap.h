#ifndef SMAP_H
#define SMAP_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * `PMap` with string keys.
 * Keys are memory managed.
 */
struct SMap; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct SMapItState; // IWYU pragma: keep
struct SMapIt {
	const char *key;
	const void *val;
	struct SMapItState *st;
};

/*
 * Optional constructor params (default)
 */
struct SMapParams {
	const bool case_insensitive; // false
	const fn_equal equal_val;    // compare key pointers
	const fn_clone alloc_val;    // assign val pointer
	const fn_free free_val;      // free
	const fn_clone clone_val;    // NOP
	const fn_str str_val;        // %p
	const bool allow_null_val;   // false
	const size_t initial;        // 10
	const size_t grow;           // 10
};

/*
 * Key/Val
 */
struct SMapPair {
	const char *key;
	const void *val;
};

/*
 * Lifecycle
 */

// construct with SMapParams defaults
const struct SMap *smap_init(void);

// construct with params
const struct SMap *smap_init_with(const struct SMapParams params);

// same params, caller frees vals when alloc_val present [alloc_val]
const struct SMap *smap_clone(const struct SMap* const from);

// same params, caller frees vals, NULL on NULL clone_val, alloc_val overrides clone_val [alloc_key, alloc_val, clone_val]
const struct SMap *smap_clone_deep(const struct SMap* const from);

// free map
void smap_free(const struct SMap* const map);

// free map and vals [free_val]
void smap_free_vals(const struct SMap* const map);

// free iterator
void smap_it_free(const struct SMapIt* const it);

/*
 * Access
 */

// return val, NULL if not present
const void *smap_get(const struct SMap* const map, const char* const key);

// true if key is present
bool smap_contains_key(const struct SMap* const map, const char* const key);

// true if val is present [equal_val]
bool smap_contains_val(const struct SMap* const map, const void* const val);

// find the first key/val match, {NULL,NULL} when no matches or NULL match
struct SMapPair smap_match(const struct SMap* const map, fn_match_str_ptr match, const void* const data);

// find the first key match, {NULL,NULL} when no matches or NULL match
struct SMapPair smap_match_key(const struct SMap* const map, fn_match_str match, const void* const data);

// find the first val match, {NULL,NULL} when no matches or NULL match
struct SMapPair smap_match_val(const struct SMap* const map, fn_match_ptr match, const void* const data);

// create an iterator, caller must smap_it_free or invoke smap_next until NULL
const struct SMapIt *smap_it(const struct SMap* const map);

// create an iterator filtering by key/val match, return NULL when no matches or NULL match
const struct SMapIt *smap_match_it(const struct SMap* const map, fn_match_str_ptr match, const void* const data);

// create an iterator filtering by key match, return NULL when no matches or NULL match
const struct SMapIt *smap_match_key_it(const struct SMap* const map, fn_match_str match, const void* const data);

// create an iterator filtering by val match, return NULL when no matches or NULL match
const struct SMapIt *smap_match_val_it(const struct SMap* const map, fn_match_ptr match, const void* const data);

// next iterator entry, NULL at end of map
const struct SMapIt *smap_it_next(const struct SMapIt* const it);

/*
 * Mutate
 */

// set key/val, return old val if overwritten [alloc_val]
const void *smap_put(const struct SMap* const map, const char* const key, const void* const val);

// set key/val if not present, return existing val if present [alloc_val]
const void *smap_put_if_absent(const struct SMap* const map, const char* const key, const void* const val);

// set key/val, free old val, return true if overwritten [alloc_val, free_val]
bool smap_put_free(const struct SMap* const map, const  char* const key, const void* const val);

// set all from key/val, returning number overwritten [alloc_val]
size_t smap_put_all(const struct SMap* const map, const struct SMap* const from);

// set all from key/val, returning number overwritten, freeing overwritten vals [alloc_val, free_val]
size_t smap_put_all_free(const struct SMap* const map, const struct SMap* const from);

// set all from key/val, returning number overwritten, NOP when NULL clone_val  [clone_val]
size_t smap_put_all_clone(const struct SMap* const map, const struct SMap* const from);

// set all from key/val, returning number overwritten, freeing overwritten vals, NOP when NULL clone_val [free_val, clone_val]
size_t smap_put_all_clone_free(const struct SMap* const map, const struct SMap* const from);

// remove val, return old val if present
const void *smap_remove(const struct SMap* const map, const char* const key);

// remove val, if removed free val and return true [free_val]
bool smap_remove_free(const struct SMap* const map, const char* const key);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses params from a [equal_val]
bool smap_equal(const struct SMap* const a, const struct SMap* const b);

/*
 * Conversion
 */

// map ordered keys, caller frees list and contents
struct SList *smap_keys_slist(const struct SMap* const map);

// map ordered keys, same params
const struct SSet *smap_keys_sset(const struct SMap* const map);

// map ordered vals, caller frees list, caller frees contents when alloc_val present [alloc_val]
struct SList *smap_vals_slist(const struct SMap* const map);

// map ordered vals, caller frees list and vals, NULL when NULL clone_val [clone_val]
struct SList *smap_vals_slist_clone(const struct SMap* const map);

// map ordered vals, same params, caller frees set, caller frees vals when alloc_val present [alloc_val]
const struct PSet *smap_vals_pset(const struct SMap* const map);

// map ordered vals, same params, caller frees set and vals, NULL on NULL clone_val or both alloc_val and clone_val [clone_val]
const struct PSet *smap_vals_pset_clone(const struct SMap* const map);


/*
 * Info
 */

// to string, user frees, format "k = str_val\n"
char *smap_str(const struct SMap* const map);

// number of entries
size_t smap_size(const struct SMap* const map);

#endif // SMAP_H

