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
struct PSetIterState; // IWYU pragma: keep
struct PSetIter {
	const void* val;
	struct PSetIterState *st;
};

// TODO consider using free_val for all _free functions

/*
 * Optional constructor params (default)
 */
struct PSetParams {
	const fn_equal equal_val; // _add, _remove, _contains, _equal         (compare val pointers)
	const fn_alloc alloc_val; // _add, _clone, _slist, must be idempotent (use key pointer)
	const fn_free free_val;   // _free_vals                               (free)
	const fn_str str_val;     // _str                                     (%p)
	const size_t initial;     // initial capacity                         (10)
	const size_t grow;        // grow capacity by                         (10)
};

/*
 * Lifecycle
 */

// construct a set with PSetParams defaults
const struct PSet *pset_init(void);

// construct a set with params
const struct PSet *pset_init_with(const struct PSetParams params);

// clone a table, NULL clone_val for shallow clone
const struct PSet *pset_clone(const struct PSet* const from, fn_clone clone_val);

// free set
void pset_free(const struct PSet* const set);

// free set and vals
void pset_free_vals(const struct PSet* const set);

// free iter
void pset_iter_free(const struct PSetIter* const iter);

/*
 * Access
 */

// true if this set contains the specified element
bool pset_contains(const struct PSet* const set, const void* const val);

// create an iterator, caller must pset_iter_free or invoke pset_next until NULL
const struct PSetIter *pset_iter(const struct PSet* const set);

// create an iterator filtering by equal_val, NULL equal_val matches all
const struct PSetIter *pset_filter_iter(const struct PSet* const set, fn_equal equal_val, const void* const data);

// next iterator value, NULL at end of set
const struct PSetIter *pset_iter_next(const struct PSetIter* const iter);

/*
 * Mutate
 */

// true if this set did not already contain the specified element
bool pset_add(const struct PSet* const set, const void* const val);

// returns value if removed
const void *pset_remove(const struct PSet* const set, const void* const val);

// shell sort in place, NULL less_than_val NOP
void pset_sort(const struct PSet* const set, fn_less_than less_than_val);

/*
 * Comparison
 */

// same length, vals equal in order, uses equal_val from a
bool pset_equal(const struct PSet* const a, const struct PSet* const b);

/*
 * Conversion
 */

// ordered val pointers, caller frees list, caller frees vals when alloc_val
struct SList *pset_slist(const struct PSet* const set);

/*
 * Info
 */

// to string, user frees, format "str_val\n"
char *pset_str(const struct PSet* const set);

// number of values
size_t pset_size(const struct PSet* const set);

#endif // PSET_H

