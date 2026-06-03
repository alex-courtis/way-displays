#ifndef SSET_H
#define SSET_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Array backed ordered string set.
 * Operations linearly traverse values.
 * NULL not permitted.
 * Not thread safe.
*/
struct SSet; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct SSetIter; // IWYU pragma: keep

/*
 * Lifecycle
 */

// construct a set with initial size 10, growing by 10 as necessary
const struct SSet *sset_init(void);

// construct a set with initial size, grow as needed, NULL on zero param
const struct SSet *sset_init_with(const size_t initial, const size_t grow, const bool case_insensitive);

// free set and vals
void sset_free(const struct SSet* const set);

// free iter
void sset_iter_free(const struct SSetIter* const iter);

/*
 * Access
 */

// true if this set contains the specified element
bool sset_contains(const struct SSet* const set, const char* const val);

// create an iterator, caller must sset_iter_free or invoke sset_next until NULL
const struct SSetIter *sset_iter(const struct SSet* const set);

// next iterator value, NULL at end of set
const struct SSetIter *sset_iter_next(const struct SSetIter* const iter);

// iterator value, NULL on NULL iter
const char *sset_iter_val(const struct SSetIter* const iter);

/*
 * Mutate
 */

// true if this set did not already contain the specified element
bool sset_add(const struct SSet* const set, const char* const val);

// true if this set contained the element
bool sset_remove(const struct SSet* const set, const char* const val);

/*
 * Comparison
 */

// same length, vals equal in order, case sensitivity is from a
bool sset_equal(const struct SSet* const a, const struct SSet* const b);

// TODO vals out of order

/*
 * Conversion
 */

// ordered val pointers to set, caller frees list only
struct SList *sset_vals_slist(const struct SSet* const set);

/*
 * Info
 */

// to string, user frees
// lines with format "%s\n"
char *sset_str(const struct SSet* const set);

// number of values
size_t sset_size(const struct SSet* const set);

// current capacity: initial + n * grow
size_t sset_capacity(const struct SSet* const set);

#endif // SSET_H

