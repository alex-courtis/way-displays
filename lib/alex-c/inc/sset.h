#ifndef SSET_H
#define SSET_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * `Pset` with string values
 * Values are memory managed.
 */
struct Sset; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct SsetItState; // IWYU pragma: keep
struct SsetIt {
	const char* val;
	struct SsetItState *st;
};

/*
 * Optional constructor params, defaults noted
 */
struct SsetParams {
	const bool case_insensitive; // false
	const size_t initial;        // 10
	const size_t grow;           // 10
};

/*
 * Lifecycle
 */

// construct a set with defaults
const struct Sset *sset_init(void);

// construct a set with params
const struct Sset *sset_init_with(const struct SsetParams params);

// clone a set
const struct Sset *sset_clone(const struct Sset* const from);

// free set
void sset_free(const struct Sset* const set);

// free iterator
void sset_it_free(const struct SsetIt* const it);

/*
 * Access
 */

// true if this set contains the specified element
bool sset_contains(const struct Sset* const set, const char* const val);

// find the first match, NULL when no match or NULL match
const void *sset_match(const struct Sset* const set, fn_2pred_str match, const void* const data);

// create an iterator, caller must sset_it_free or invoke pset_next until NULL
const struct SsetIt *sset_it(const struct Sset* const set);

// create an iterator filtering by match, return NULL when no matches or NULL match
const struct SsetIt *sset_match_it(const struct Sset* const set, fn_2pred_str match, const void* const data);

// next iterator value, NULL at end of set
const struct SsetIt *sset_it_next(const struct SsetIt* const it);

/*
 * Mutate
 */

// add if the set does not contain val, return true if added
bool sset_add(const struct Sset* const set, const char* const val);

// add from vals not contained in the set, return number added
size_t sset_add_all(const struct Sset* const set, const struct Sset* const from);

// if the set contains val, remove it, free it and return true
bool sset_remove(const struct Sset* const set, const char* const val);

// remove vals contained in from, return number removed
size_t sset_remove_all(const struct Sset* const set, const struct Sset* const from);

// shell sort in place
void sset_sort(const struct Sset* const set);

/*
 * Comparison
 */

// same length, vals equal in order, case sensitivity is from a
bool sset_equal(const struct Sset* const a, const struct Sset* const b);

/*
 * Conversion
 */

// set ordered vals, caller frees list and vals
struct Pslist *sset_pslist(const struct Sset* const set);

/*
 * Info
 */

// to string, user frees, format "%s\n"
char *sset_str(const struct Sset* const set);

// number of values
size_t sset_size(const struct Sset* const set);

#endif // SSET_H

