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
struct PSet; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct PSetItState; // IWYU pragma: keep
struct PSetIt {
	const void* val;
	struct PSetItState *st;
};

/*
 * Optional constructor params (default)
 */
struct PSetParams {
	const fn_equal equal_val; // compare val pointers
	const fn_alloc alloc_val; // use key pointer
	const fn_free free_val;   // free
	const fn_clone clone_val; // use key pointer
	const fn_str str_val;     // %p
	const size_t initial;     // 10
	const size_t grow;        // 10
};

/*
 * Lifecycle
 */

// construct with PSetParams defaults
const struct PSet *pset_init(void);

// construct with params
const struct PSet *pset_init_with(const struct PSetParams params);

// clone, setting val pointers
const struct PSet *pset_clone_shallow(const struct PSet* const from);

// clone, empty when NULL clone_val [clone_val]
const struct PSet *pset_clone_deep(const struct PSet* const from);

// free set
void pset_free(const struct PSet* const set);

// free set and vals [free_val]
void pset_free_vals(const struct PSet* const set);

// free iterator
void pset_it_free(const struct PSetIt* const it);

/*
 * Access
 */

// true if this set contains the specified element [equal_val]
bool pset_contains(const struct PSet* const set, const void* const val);

// find the first match, NULL when no match or NULL match
const void *pset_match(const struct PSet* const set, fn_match_val match, const void* const data);

// create an iterator, caller must pset_it_free or invoke pset_next until NULL
const struct PSetIt *pset_it(const struct PSet* const set);

// create an iterator filtering by match, return NULL when no matches or NULL match
const struct PSetIt *pset_match_it(const struct PSet* const set, fn_match_val match, const void* const data);

// next iterator value, NULL at end of set
const struct PSetIt *pset_it_next(const struct PSetIt* const it);

/*
 * Mutate
 */

// add if the set does not contain val, return true if added [equal_val, alloc_val]
bool pset_add(const struct PSet* const set, const void* const val);

// if the set contains val, remove it and return true [equal_val, alloc_val]
bool pset_remove(const struct PSet* const set, const void* const val);

// if the set contains val, remove it, free it and return true [equal_val, alloc_val, free_val]
bool pset_remove_free(const struct PSet* const set, const void* const val);

// shell sort in place, NULL less_than_val NOP
void pset_sort(const struct PSet* const set, fn_less_than less_than_val);

/*
 * Comparison
 */

// same length, vals equal in order, uses params from a [equal_val]
bool pset_equal(const struct PSet* const a, const struct PSet* const b);

/*
 * Conversion
 */

// ordered vals, caller frees list only
struct SList *pset_slist_shallow(const struct PSet* const set);

// ordered vals, caller frees list and vals, empty when NULL clone_val [clone_val]
struct SList *pset_slist_deep(const struct PSet* const set);

/*
 * Info
 */

// to string, user frees, format "str_val\n"
char *pset_str(const struct PSet* const set);

// number of values
size_t pset_size(const struct PSet* const set);

#endif // PSET_H

