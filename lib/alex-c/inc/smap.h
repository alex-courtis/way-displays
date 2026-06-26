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
	const fn_alloc alloc_val;    // assign val pointer
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
 * match against supplied data
 */
typedef bool (*fn_match_smap)(const char* const key, const void* const val, const void* const data);

/*
 * Lifecycle
 */

// construct with SMapParams defaults
const struct SMap *smap_init(void);

// construct with params
const struct SMap *smap_init_with(const struct SMapParams params);

// clone, setting val pointers
const struct SMap *smap_clone_shallow(const struct SMap* const from);

// clone, empty when NULL clone_val [clone_val]
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

// find the first match, {NULL,NULL} when no matches or NULL match
struct SMapPair smap_match(const struct SMap* const map, fn_match_smap match, const void* const data);

// create an iterator, caller must smap_it_free or invoke smap_next until NULL
const struct SMapIt *smap_it(const struct SMap* const map);

// create an iterator filtering by match, return NULL when no matches or NULL match
const struct SMapIt *smap_match_it(const struct SMap* const map, fn_match_smap match, const void* const data);

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

// ordered keys, caller frees list and vals
struct SList *smap_keys_slist_deep(const struct SMap* const map);

// ordered vals, caller frees list only
struct SList *smap_vals_slist_shallow(const struct SMap* const map);

// ordered vals, caller frees list and vals, empty when NULL clone_val [clone_val]
struct SList *smap_vals_slist_deep(const struct SMap* const map);

/*
 * Info
 */

// to string, user frees, format "k = str_val\n"
char *smap_str(const struct SMap* const map);

// number of entries
size_t smap_size(const struct SMap* const map);

#endif // SMAP_H

