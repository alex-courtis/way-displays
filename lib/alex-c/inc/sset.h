#ifndef SSET_H
#define SSET_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * `PSet` with string values
 * Values are memory managed.
 */
struct SSet; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct SSetItState; // IWYU pragma: keep
struct SSetIt {
	const char* val;
	struct SSetItState *st;
};

/*
 * Optional constructor params, defaults noted
 */
struct SSetParams {
	const bool case_insensitive; // false
	const size_t initial;        // 10
	const size_t grow;           // 10
};

/*
 * Lifecycle
 */

// construct a set with defaults
const struct SSet *sset_init(void);

// construct a set with params
const struct SSet *sset_init_with(const struct SSetParams params);

// clone a set
const struct SSet *sset_clone(const struct SSet* const from);

// free set
void sset_free(const struct SSet* const set);

// free iterator
void sset_it_free(const struct SSetIt* const it);

/*
 * Access
 */

// true if this set contains the specified element
bool sset_contains(const struct SSet* const set, const char* const val);

// find the first match, NULL when no match or NULL match
const void *sset_match(const struct SSet* const set, fn_match_str match, const void* const data);

// create an iterator, caller must sset_it_free or invoke pset_next until NULL
const struct SSetIt *sset_it(const struct SSet* const set);

// create an iterator filtering by match, return NULL when no matches or NULL match
const struct SSetIt *sset_match_it(const struct SSet* const set, fn_match_str match, const void* const data);

// next iterator value, NULL at end of set
const struct SSetIt *sset_it_next(const struct SSetIt* const it);

/*
 * Mutate
 */

// add if the set does not contain val, return true if added
bool sset_add(const struct SSet* const set, const char* const val);

// if the set contains val, remove it, free it and return true
bool sset_remove(const struct SSet* const set, const char* const val);

// shell sort in place
void sset_sort(const struct SSet* const set);

/*
 * Comparison
 */

// same length, vals equal in order, case sensitivity is from a
bool sset_equal(const struct SSet* const a, const struct SSet* const b);

/*
 * Conversion
 */

// set ordered vals, caller frees list and vals
struct SList *sset_slist_deep(const struct SSet* const set);

/*
 * Info
 */

// to string, user frees, format "%s\n"
char *sset_str(const struct SSet* const set);

// number of values
size_t sset_size(const struct SSet* const set);

#endif // SSET_H

