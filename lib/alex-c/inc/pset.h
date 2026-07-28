#ifndef PSET_H
#define PSET_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * Array backed pointer set.
 * Entries preserve insertion order.
 * Operations linearly traverse values.
 * NULL not permitted.
 */
struct Pset; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct PsetItState; // IWYU pragma: keep
struct PsetIt {
	const void* val;
	struct PsetItState *st;
};

/*
 * Filter, must match all when multiple predicates specified, empty filter matches anything
 */
struct PsetFilter {
	// test vals
	fn_pred_p val;

	// test vals against user data
	const void *data;
	fn_pred_pp val_data;
};

/*
 * Optional constructor params (default)
 */
struct PsetParams {
	const fn_equal equal_val; // compare val pointers
	const fn_clone alloc_val; // use pointer
	const fn_free free_val;   // free
	const fn_clone clone_val; // use pointer
	const fn_str str_val;     // %p
	const size_t initial;     // 10
	const size_t grow;        // 10
};

/*
 * Lifecycle
 */

// construct with PsetParams defaults
const struct Pset *pset_init(void);

// construct with params
const struct Pset *pset_init_with(const struct PsetParams params);

// same params, caller frees vals when alloc_val present [alloc_val]
const struct Pset *pset_clone(const struct Pset* const from);

// set ordered vals, caller frees vals, NULL when NULL clone_val [clone_val]
const struct Pset *pset_clone_deep(const struct Pset* const from);

// free set
void pset_free(const struct Pset* const set);

// free set and vals [free_val]
void pset_free_vals(const struct Pset* const set);

// free iterator
void pset_it_free(const struct PsetIt* const it);

/*
 * Access
 */

// true if this set contains the specified element [equal_val]
bool pset_contains(const struct Pset* const set, const void* const val);

// element at zero indexed position
const void *pset_at(const struct Pset* const set, const size_t i);

// find the first, NULL when no matches, first entry when empty filter
const void *pset_find(const struct Pset* const set, const struct PsetFilter filter);

// create an iterator, caller must pset_it_free or invoke pset_next until NULL
const struct PsetIt *pset_it(const struct Pset* const set);

// create a filtering iterator, return NULL when no matches, first entry when empty filter
const struct PsetIt *pset_filter_it(const struct Pset* const set, const struct PsetFilter filter);

// next iterator val, NULL at end of set
const struct PsetIt *pset_it_next(const struct PsetIt* const it);

/*
 * Mutate
 */

// add if the set does not contain val, return true if added [equal_val, alloc_val]
bool pset_add(const struct Pset* const set, const void* const val);

// add if the set does not contain val, return true if added, NOP when NULL clone_val [equal_val, clone_val]
bool pset_add_clone(const struct Pset* const set, const void* const val);

// add from vals not contained in the set, return number added [equal_val, alloc_val]
size_t pset_add_all(const struct Pset* const set, const struct Pset* const from);

// add from vals not contained in the set, return number added, NOP when NULL clone_val [equal_val, clone_val]
size_t pset_add_all_clone(const struct Pset* const set, const struct Pset* const from);

// remove val, return val if removed [equal_val]
const void *pset_remove(const struct Pset* const set, const void* const val);

// remove and free val, return true if removed [equal_val, alloc_val, free_val]
bool pset_remove_free(const struct Pset* const set, const void* const val);

// remove all vals, returning number removed
size_t pset_remove_all(const struct Pset* const set);

// remove all vals and free, returning number removed [free_val]
size_t pset_remove_all_free(const struct Pset* const set);

// remove vals contained in, return number removed [equal_val]
size_t pset_remove_in(const struct Pset* const set, const struct Pset* const in);

// remove and free vals contained in, return number removed [equal_val, free_val]
size_t pset_remove_in_free(const struct Pset* const set, const struct Pset* const in);

// remove the it.val, return val if removed, it is unusable, pset_it_next must be called
const void *pset_it_remove(const struct PsetIt* const it);

// remove and free the it.val, return true if removed, it is unusable, pset_it_next must be called [free_val]
bool pset_it_remove_free(const struct PsetIt* const it);

// shell sort in place, NULL less_than_val NOP
void pset_sort(const struct Pset* const set, fn_less_than less_than_val);

/*
 * Comparison
 */

// same length, contains same vals, uses params from a [equal_val]
bool pset_equal(const struct Pset* const a, const struct Pset* const b);

// same length, vals equal in order, uses params from a [equal_val]
bool pset_equal_ordered(const struct Pset* const a, const struct Pset* const b);

/*
 * Conversion
 */

// set ordered vals, same params, caller frees contents when alloc_val present [alloc_val]
const struct Plist *pset_plist(const struct Pset* const set);

// set ordered vals, same params, caller frees contents, NULL when NULL clone_val [clone_val]
const struct Plist *pset_plist_clone(const struct Pset* const set);

/*
 * Info
 */

// to string, user frees, format "str_val\n"
char *pset_str(const struct Pset* const set);

// number of values
size_t pset_size(const struct Pset* const set);

#endif // PSET_H

