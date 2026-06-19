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
struct SMapSIterState; // IWYU pragma: keep
struct SMapSIter {
	const char *key;
	const char *val;
	struct SMapSIterState *st;
};

/*
 * Optional constructor params (default)
 */
struct SMapSParams {
	const bool case_insensitive_key; // (false)
	const bool case_insensitive_val; // (false)
	const size_t initial;            // (10)
	const size_t grow;               // (10)
};

/*
 * Lifecycle
 */

// construct with SMapSParams defaults
const struct SMapS *smaps_init(void);

// construct with params
const struct SMapS *smaps_init_with(const struct SMapSParams params);

// clone
const struct SMapS *smaps_clone_deep(const struct SMapS* const from);

// free map keys and values
void smaps_free_vals(const struct SMapS* const tab);

// free iter
void smaps_iter_free(const struct SMapSIter* const iter);

/*
 * Access
 */

// return val, NULL if not present
const char *smaps_get(const struct SMapS* const tab, const char* const key);

// true if key is present
bool smaps_contains_key(const struct SMapS* const tab, const char* const key);

// create an iterator, caller must smaps_iter_free or invoke smaps_next until NULL
const struct SMapSIter *smaps_iter(const struct SMapS* const tab);

// create an iterator filtering by equal_key and equal_val, NULL tests match all
const struct SMapSIter *smaps_filter_iter(const struct SMapS* const tab, fn_equal equal_key, fn_equal equal_val, const void* const data);

// next iterator entry, NULL at end of map
const struct SMapSIter *smaps_iter_next(const struct SMapSIter* const iter);

/*
 * Mutate
 */

// set key/val, return true if overwritten
bool smaps_put_free(const struct SMapS* const tab, const char* const key, const char* const val);

// set key/val if not present, return true if overwritten
bool smaps_put_if_absent(const struct SMapS* const tab, const char* const key, const char* const val);

// remove val, return true if removed
bool smaps_remove_free(const struct SMapS* const tab, const char* const key);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses case from a
bool smaps_equal(const struct SMapS* const a, const struct SMapS* const b);

/*
 * Conversion
 */

// ordered vals, caller frees list and vals
struct SList *smaps_keys_slist_deep(const struct SMapS* const tab);

// ordered vals, caller frees list and vals
struct SList *smaps_vals_slist_deep(const struct SMapS* const tab);

/*
 * Info
 */

// to string, user frees, format "k = v\n"
char *smaps_str(const struct SMapS* const tab);

// number of entries
size_t smaps_size(const struct SMapS* const tab);

#endif // SMAPS_H

