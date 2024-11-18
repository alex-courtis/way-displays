#ifndef OSET_H
#define OSET_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * Array backed ordered set.
 * Operations linearly traverse values.
 * NULL not permitted.
 * Not thread safe.
*/
struct OSet;

/*
 * Entry iterator.
 */
struct OSetIter {
	const void* const val;
};

/*
 * Lifecycle
 */

// construct a set with initial size, grow as needed, NULL on zero param
const struct OSet *oset_init(const size_t initial, const size_t grow);

// free set
void oset_free(const void* const set);

// free map and vals, NULL fn_free_val uses free()
void oset_free_vals(const struct OSet* const set, fn_free_val);

// free iter
void oset_iter_free(const struct OSetIter* const iter);

/*
 * Access
 */

// true if this set contains the specified element
bool oset_contains(const struct OSet* const set, const void* const val);

// create an iterator, caller must oset_iter_free or invoke oset_next until NULL
const struct OSetIter *oset_iter(const struct OSet* const set);

// next iterator value, NULL at end of set
const struct OSetIter *oset_next(const struct OSetIter* const iter);

/*
 * Mutate
 */

// true if this set did not already contain the specified element
bool oset_add(const struct OSet* const set, const void* const val);

// true if this set contained the element
bool oset_remove(const struct OSet* const set, const void* const val);

/*
 * Comparison
 */

// same length, vals equal in order, NULL equal compares pointers
bool oset_equal(const struct OSet* const a, const struct OSet* const b, bool (*equal)(const void *a, const void *b));

/*
 * Conversion
 */

// ordered val pointers to table, caller frees list only
struct SList *oset_vals_slist(const struct OSet* const set);

/*
 * Info
 */

// number of values
size_t oset_size(const struct OSet* const set);

// current capacity: initial + n * grow
size_t oset_capacity(const struct OSet* const set);

#endif // OSET_H

