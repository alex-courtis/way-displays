#ifndef SMAPS_H
#define SMAPS_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * `PMap` with string keys and vals.
 * Keys and values are memory managed.
 */
struct SMapS; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct SMapSItState; // IWYU pragma: keep
struct SMapSIt {
	const char *key;
	const char *val;
	struct SMapSItState *st;
};

/*
 * Optional constructor params (default)
 */
struct SMapSParams {
	const bool case_insensitive_key; // (false)
	const bool case_insensitive_val; // (false)
	const bool allow_null_val;       // false
	const size_t initial;            // (10)
	const size_t grow;               // (10)
};

/*
 * Key/Val
 */
struct SMapSPair {
	const char *key;
	const char *val;
};

/*
 * Lifecycle
 */

// construct with SMapSParams defaults
const struct SMapS *smaps_init(void);

// construct with params
const struct SMapS *smaps_init_with(const struct SMapSParams params);

// clone
const struct SMapS *smaps_clone(const struct SMapS* const from);

// free map
void smaps_free(const struct SMapS* const map);

// free iterator
void smaps_it_free(const struct SMapSIt* const it);

/*
 * Access
 */

// return val, NULL if not present
const char *smaps_get(const struct SMapS* const map, const char* const key);

// true if key is present
bool smaps_contains_key(const struct SMapS* const map, const char* const key);

// true if val is present
bool smaps_contains_val(const struct SMapS* const map, const char* const val);

// find the first key/val match, {NULL,NULL} when no matches or NULL match
struct SMapSPair smaps_match(const struct SMapS* const map, fn_match_str_str match, const void* const data);

// find the first key match, {NULL,NULL} when no matches or NULL match
struct SMapSPair smaps_match_key(const struct SMapS* const map, fn_match_str match, const void* const data);

// find the first val match, {NULL,NULL} when no matches or NULL match
struct SMapSPair smaps_match_val(const struct SMapS* const map, fn_match_str match, const void* const data);

// create an iterator, caller must smaps_it_free or invoke smaps_next until NULL
const struct SMapSIt *smaps_it(const struct SMapS* const map);

// create an iterator filtering by key/val match, return NULL when no matches or NULL match
const struct SMapSIt *smaps_match_it(const struct SMapS* const map, fn_match_str_str match, const void* const data);

// create an iterator filtering by val match, return NULL when no matches or NULL match
const struct SMapSIt *smaps_match_val_it(const struct SMapS* const map, fn_match_str match, const void* const data);

// next iterator entry, NULL at end of map
const struct SMapSIt *smaps_it_next(const struct SMapSIt* const it);

/*
 * Mutate
 */

// set key/val, return true if overwritten
bool smaps_put(const struct SMapS* const map, const char* const key, const char* const val);

// set key/val if not present, return true if overwritten
bool smaps_put_if_absent(const struct SMapS* const map, const char* const key, const char* const val);

// remove val, return true if removed
bool smaps_remove(const struct SMapS* const map, const char* const key);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses case from a
bool smaps_equal(const struct SMapS* const a, const struct SMapS* const b);

/*
 * Conversion
 */

// map ordered vals, caller frees list and vals
struct SList *smaps_keys_slist_deep(const struct SMapS* const map);

// map ordered keys, same parameters
const struct SSet *smaps_keys_sset(const struct SMapS* const map);

// map ordered vals, caller frees list and vals
struct SList *smaps_vals_slist_deep(const struct SMapS* const map);

// map ordered vals, same parameters
const struct SSet *smaps_vals_sset(const struct SMapS* const map);

/*
 * Info
 */

// to string, user frees, format "k = v\n"
char *smaps_str(const struct SMapS* const map);

// number of entries
size_t smaps_size(const struct SMapS* const map);

#endif // SMAPS_H

