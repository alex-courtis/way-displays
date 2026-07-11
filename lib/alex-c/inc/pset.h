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
 * Optional constructor params (default)
 */
struct PsetParams {
	const fn_equal equal_val; // compare val pointers
	const fn_clone alloc_val; // use key pointer
	const fn_free free_val;   // free
	const fn_clone clone_val; // use key pointer
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

// find the first, NULL when no match or NULL match
const void *pset_find(const struct Pset* const set, fn_2pred pred_val, const void* const data);

// create an iterator, caller must pset_it_free or invoke pset_next until NULL
const struct PsetIt *pset_it(const struct Pset* const set);

// create an iterator filtering by pred, return NULL when no matches or NULL match
const struct PsetIt *pset_filter_it(const struct Pset* const set, fn_2pred pred_val, const void* const data);

// next iterator value, NULL at end of set
const struct PsetIt *pset_it_next(const struct PsetIt* const it);

/*
 * Mutate
 */

// add if the set does not contain val, return true if added [equal_val, alloc_val]
bool pset_add(const struct Pset* const set, const void* const val);

// add from vals not contained in the set, return number added [equal_val, alloc_val]
size_t pset_add_all(const struct Pset* const set, const struct Pset* const from);

// add from vals not contained in the set, return number added, NOP when NULL clone_val [equal_val, clone_val]
size_t pset_add_all_clone(const struct Pset* const set, const struct Pset* const from);

// if the set contains val, remove it and return true [equal_val, alloc_val]
bool pset_remove(const struct Pset* const set, const void* const val);

// if the set contains val, remove it, free it and return true [equal_val, alloc_val, free_val]
bool pset_remove_free(const struct Pset* const set, const void* const val);

// remove vals contained in from, return number removed [equal_val]
size_t pset_remove_all(const struct Pset* const set, const struct Pset* const from);

// remove and free vals contained in from, return number removed [equal_val, free_val]
size_t pset_remove_all_free(const struct Pset* const set, const struct Pset* const from);

// shell sort in place, NULL less_than_val NOP
void pset_sort(const struct Pset* const set, fn_less_than less_than_val);

/*
 * Comparison
 */

// same length, vals equal in order, uses params from a [equal_val]
bool pset_equal(const struct Pset* const a, const struct Pset* const b);

/*
 * Conversion
 */

// set ordered vals, caller frees list, caller frees contents when alloc_val present [alloc_val]
struct Pslist *pset_pslist(const struct Pset* const set);

// set ordered vals, caller frees list and vals, NULL when NULL clone_val [clone_val]
struct Pslist *pset_pslist_clone(const struct Pset* const set);

/*
 * Info
 */

// to string, user frees, format "str_val\n"
char *pset_str(const struct Pset* const set);

// number of values
size_t pset_size(const struct Pset* const set);

#endif // PSET_H

