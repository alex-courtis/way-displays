#ifndef IMAP_H
#define IMAP_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * `PMap` with `size_t` keys
 */
struct IMap; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct IMapItState; // IWYU pragma: keep
struct IMapIt {
	size_t key;
	const void *val;
	struct IMapItState *st;
};

/*
 * Key/Val
 */
struct IMapPair {
	size_t key;
	const void *val;
};

/*
 * Optional constructor params (default)
 */
struct IMapParams {
	const fn_equal equal_val; // compare key pointers
	const fn_alloc alloc_val; // assign key pointer
	const fn_free free_val;   // free
	const fn_clone clone_val; // NOP
	const fn_str str_val;     // %p
	const size_t initial;     // 10
	const size_t grow;        // 10
};

/*
 * match against supplied data
 */
typedef bool (*fn_match_imap)(const size_t key, const void* const val, const void* const data);

/*
 * Lifecycle
 */

// construct with IMapParams defaults
const struct IMap *imap_init(void);

// construct with params
const struct IMap *imap_init_with(const struct IMapParams params);

// clone, setting val pointers
const struct IMap *imap_clone_shallow(const struct IMap* const from);

// clone, empty when NULL clone_val [clone_val]
const struct IMap *imap_clone_deep(const struct IMap* const from);

// free map
void imap_free(const struct IMap* const map);

// free map and vals [free_val]
void imap_free_vals(const struct IMap* const map);

// free iterator
void imap_it_free(const struct IMapIt* const it);

/*
 * Access
 */

// return val, NULL if not present
const void *imap_get(const struct IMap* const map, const size_t key);

// true if key is present
bool imap_contains_key(const struct IMap* const map, const size_t key);

// find the first match, {0,NULL} when no matches or NULL match
struct IMapPair imap_match(const struct IMap* const map, fn_match_imap match, const void* const data);

// create an iterator, caller must imap_it_free or invoke imap_next until NULL
const struct IMapIt *imap_it(const struct IMap* const map);

// create an iterator filtering by match, return NULL when no matches or NULL match
const struct IMapIt *imap_match_it(const struct IMap* const map, fn_match_imap match, const void* const data);

// next iterator entry, NULL at end of map
const struct IMapIt *imap_it_next(const struct IMapIt* const it);

/*
 * Mutate
 */

// set key/val, return old val if overwritten [alloc_val]
const void *imap_put(const struct IMap* const map, const size_t key, const void* const val);

// set key/val if not present, return existing val if present [alloc_val]
const void *imap_put_if_absent(const struct IMap* const map, const size_t key, const void* const val);

// set key/val, free old val, return true if overwritten [alloc_val, free_val]
bool imap_put_free(const struct IMap* const map, const size_t key, const char* const val);

// remove val, return old val if present
const void *imap_remove(const struct IMap* const map, const size_t key);

// remove val, if removed free val and return true [free_val]
bool imap_remove_free(const struct IMap* const map, const size_t key);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses params from a [equal_val]
bool imap_equal(const struct IMap* const a, const struct IMap* const b);

/*
 * Conversion
 */

// ordered vals, caller frees list only
struct SList *imap_vals_slist_shallow(const struct IMap* const map);

// ordered vals, caller frees list and vals, empty when NULL clone_val [clone_val]
struct SList *imap_vals_slist_deep(const struct IMap* const map);

/*
 * Info
 */

// to string, user frees, format "k = str_val\n"
char *imap_str(const struct IMap* const map);

// number of entries
size_t imap_size(const struct IMap* const map);

#endif // IMAP_H

