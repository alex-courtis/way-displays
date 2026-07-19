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
 * Filter, must match all when multiple predicates specified, empty filter matches anything
 */
struct SsetFilter {
	// test vals
	fn_pred_str val;

	// test vals against user data
	const void *data;
	fn_2pred_str val_data;
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

// element at zero indexed position
const char *sset_at(const struct Sset* const set, const size_t i);

// find the first, NULL when no match or NULL match
const void *sset_find(const struct Sset* const set, const struct SsetFilter filter);

// create an iterator, caller must sset_it_free or invoke pset_next until NULL
const struct SsetIt *sset_it(const struct Sset* const set);

// create a filtering iterator, return NULL when no matches, caller must sset_it_free or invoke sset_next until NULL
const struct SsetIt *sset_filter_it(const struct Sset* const set, const struct SsetFilter filter);

// next iterator val, NULL at end of set
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

// remove all vals, returning number removed
size_t sset_remove_all(const struct Sset* const set);

// remove vals contained in, return number removed
size_t sset_remove_in(const struct Sset* const set, const struct Sset* const in);

// remove the it.val, it is unusable, sset_it_next must be called
void sset_it_remove(const struct SsetIt* const it);

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

