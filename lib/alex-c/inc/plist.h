#ifndef PLIST_H
#define PLIST_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * Array backed pointer list.
 * Entries preserve insertion order.
 * Operations linearly traverse values.
 */
struct Plist; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct PlistItState; // IWYU pragma: keep
struct PlistIt {
	const void* val;
	struct PlistItState *st;
};

/*
 * Filter, must match all when multiple predicates specified, empty filter matches anything
 */
struct PlistFilter {
	// test vals
	fn_pred_p val;

	// test vals against user data
	const void *data;
	fn_pred_pp val_data;
};

/*
 * Optional constructor params (default)
 */
struct PlistParams {
	const fn_equal equal_val;  // compare val pointers
	const fn_clone alloc_val;  // use val pointer
	const fn_free free_val;    // free
	const fn_clone clone_val;  // use val pointer
	const fn_str str_val;      // %p
	const bool allow_null_val; // false
	const size_t initial;      // 10
	const size_t grow;         // 10
};

/*
 * Lifecycle
 */

// construct with PlistParams defaults
const struct Plist *plist_init(void);

// construct with params
const struct Plist *plist_init_with(const struct PlistParams params);

// same params, caller frees vals when alloc_val present [alloc_val]
const struct Plist *plist_clone(const struct Plist* const from);

// list ordered vals, caller frees vals, NULL when NULL clone_val [clone_val]
const struct Plist *plist_clone_deep(const struct Plist* const from);

// free list
void plist_free(const struct Plist* const list);

// free list and vals [free_val]
void plist_free_vals(const struct Plist* const list);

// free iterator
void plist_it_free(const struct PlistIt* const it);

/*
 * Access
 */

// true if this list contains the specified element [equal_val]
bool plist_contains(const struct Plist* const list, const void* const val);

// put first index of val in index if present, 0 and return false if not present [equal_val]
bool plist_index_of(size_t *index, const struct Plist* const list, const void* const val);

// element at zero indexed position
const void *plist_at(const struct Plist* const list, const size_t i);

// find the first, NULL when no matches or allow_null_val and NULL present, first entry when empty filter
const void *plist_find(const struct Plist* const list, const struct PlistFilter filter);

// create an iterator at the start, caller must plist_it_free or invoke plist_next/prev until NULL
const struct PlistIt *plist_it_start(const struct Plist* const list);

// create an iterator at the end
const struct PlistIt *plist_it_end(const struct Plist* const list);

// create a filtering iterator, return NULL when no matches, first entry when empty filter
const struct PlistIt *plist_filter_it_start(const struct Plist* const list, const struct PlistFilter filter);

// create a filtering iterator at the end of the list, return NULL when no matches, last entry when empty filter
const struct PlistIt *plist_filter_it_end(const struct Plist* const list, const struct PlistFilter filter);

// next iterator val, NULL at end of list
const struct PlistIt *plist_it_next(const struct PlistIt* const it);

// prev iterator val, NULL at beginning of list
const struct PlistIt *plist_it_prev(const struct PlistIt* const it);

/*
 * Mutate
 */

// add at index, appends when index >= size, return true if added [alloc_val]
bool plist_insert(const struct Plist* const list, size_t index, const void* const val);

// add to end, return true if added [alloc_val]
bool plist_append(const struct Plist* const list, const void* const val);

// add to start, return true if added [alloc_val]
bool plist_prepend(const struct Plist* const list, const void* const val);

// replace val at index and return it, NOP when index >= size [alloc_val]
const void *plist_replace(const struct Plist* const list, size_t index, const void* const val);

// replace val at index and free it, return true if replaced, NOP when index >= size [alloc_val]
bool plist_replace_free(const struct Plist* const list, size_t index, const void* const val);

// add from vals, return number added [alloc_val]
size_t plist_append_all(const struct Plist* const list, const struct Plist* const from);

// add from vals, return number added, NOP when NULL clone_val [equal_val, clone_val]
size_t plist_append_all_clone(const struct Plist* const list, const struct Plist* const from);

// if the list contains val, remove the first occurrence and return it [equal_val]
const void *plist_remove(const struct Plist* const list, const void* const val);

// if the list contains val, remove the first occurrence and free it, return true if removed [equal_val, free_val]
bool plist_remove_free(const struct Plist* const list, const void* const val);

// remove val at i, return val if removed
const void *plist_remove_at(const struct Plist* const list, const size_t i);

// remove and free val at i, return true if removed, NOP when index >= size [free_val]
bool plist_remove_at_free(const struct Plist* const list, const size_t i);

// remove all vals, returning number removed
size_t plist_remove_all(const struct Plist* const list);

// remove all vals and free, returning number removed [free_val]
size_t plist_remove_all_free(const struct Plist* const list);

// remove vals contained in, return number removed [equal_val]
size_t plist_remove_in(const struct Plist* const map, const struct Plist* const in);

// remove and free vals contained in, return number removed [equal_val, free_val]
size_t plist_remove_in_free(const struct Plist* const map, const struct Plist* const in);

// remove the it.val, return val if removed, it is unusable, plist_it_next or plist_it_prev must be called
const void *plist_it_remove(const struct PlistIt* const it);

// remove and free the it.val, return true if removed, it is unusable, plist_it_next or plist_it_prev must be called [free_val]
bool plist_it_remove_free(const struct PlistIt* const it);

// shell sort in place, NULL less_than_val NOP
void plist_sort(const struct Plist* const list, fn_less_than less_than_val);

/*
 * Comparison
 */

// same length, vals equal in order, uses params from a [equal_val]
bool plist_equal(const struct Plist* const a, const struct Plist* const b);

// same length, vals equal in order, uses params from a [equal_val]
bool plist_equal_ordered(const struct Plist* const a, const struct Plist* const b);

/*
 * Info
 */

// to string, user frees, format "str_val\n"
char *plist_str(const struct Plist* const list);

// number of values
size_t plist_size(const struct Plist* const list);

#endif // PLIST_H

