#ifndef SMAPI_H
#define SMAPI_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * `PMap` with string keys and size_t vals.
 * Keys are memory managed.
 */
struct SMapI; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct SMapIItState; // IWYU pragma: keep
struct SMapIIt {
	const char *key;
	size_t val;
	struct SMapIItState *st;
};

/*
 * Optional constructor params (default)
 */
struct SMapIParams {
	const bool case_insensitive_key; // (false)
	const size_t initial;            // (10)
	const size_t grow;               // (10)
};

/*
 * Key/Val
 */
struct SMapIPair {
	const char *key;
	size_t val;
};

/*
 * Lifecycle
 */

// construct with SMapIParams defaults
const struct SMapI *smapi_init(void);

// construct with params
const struct SMapI *smapi_init_with(const struct SMapIParams params);

// clone
const struct SMapI *smapi_clone(const struct SMapI* const from);

// free map
void smapi_free(const struct SMapI* const map);

// free iterator
void smapi_it_free(const struct SMapIIt* const it);

/*
 * Access
 */

// return val, will return 0 if not present
size_t smapi_get(const struct SMapI* const map, const char* const key);

// populate *val with val if present, set *val to 0 return false if not present
bool smapi_getp(size_t* val, const struct SMapI* const map, const char* const key);

// true if key is present
bool smapi_contains_key(const struct SMapI* const map, const char* const key);

// true if val is present
bool smapi_contains_val(const struct SMapI* const map, const size_t val);

// find the first key/val match, {NULL,0} when no matches or NULL match
struct SMapIPair smapi_match(const struct SMapI* const map, fn_match_str_size_t match, const void* const data);

// find the first key match, {NULL,0} when no matches or NULL match
struct SMapIPair smapi_match_key(const struct SMapI* const map, fn_match_str match, const void* const data);

// find the first val match, {NULL,0} when no matches or NULL match
struct SMapIPair smapi_match_val(const struct SMapI* const map, fn_match_size_t match, const void* const data);

// create an iterator, caller must smapi_it_free or invoke smapi_next until NULL
const struct SMapIIt *smapi_it(const struct SMapI* const map);

// create an iterator filtering by key/val match, return NULL when no matches or NULL match
const struct SMapIIt *smapi_match_it(const struct SMapI* const map, fn_match_str_size_t match, const void* const data);

// create an iterator filtering by val match, return NULL when no matches or NULL match
const struct SMapIIt *smapi_match_val_it(const struct SMapI* const map, fn_match_size_t match, const void* const data);

// next iterator entry, NULL at end of map
const struct SMapIIt *smapi_it_next(const struct SMapIIt* const it);

/*
 * Mutate
 */

// set key/val, return true if overwritten
bool smapi_put(const struct SMapI* const map, const char* const key, const size_t val);

// set key/val if not present, return true if overwritten
bool smapi_put_if_absent(const struct SMapI* const map, const char* const key, const size_t val);

// remove val, return true if removed
bool smapi_remove(const struct SMapI* const map, const char* const key);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses case from a
bool smapi_equal(const struct SMapI* const a, const struct SMapI* const b);

/*
 * Conversion
 */

// map ordered vals, caller frees list and vals
struct SList *smapi_keys_slist_deep(const struct SMapI* const map);

// map ordered keys, same parameters
const struct SSet *smapi_keys_sset(const struct SMapI* const map);

/*
 * Info
 */

// to string, user frees, format "k = v\n"
char *smapi_str(const struct SMapI* const map);

// number of entries
size_t smapi_size(const struct SMapI* const map);

#endif // SMAPI_H

