#ifndef SLIST_H
#define SLIST_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * `Plist` with string values
 * Values are memory managed.
 */
struct Slist; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct SlistItState; // IWYU pragma: keep
struct SlistIt {
	const void* val;
	struct SlistItState *st;
};

/*
 * Filter, must match all when multiple predicates specified, empty filter matches anything
 */
struct SlistFilter {
	// test vals
	fn_pred_p val;

	// test vals against user data
	const void *data;
	fn_pred_sp val_data;
};

/*
 * Optional constructor params (default)
 */
struct SlistParams {
	const bool allow_null_val;   // false
	const bool case_insensitive; // false
	const size_t initial;        // 10
	const size_t grow;           // 10
};

/*
 * Lifecycle
 */

// construct with SlistParams defaults
const struct Slist *slist_init(void);

// construct with params
const struct Slist *slist_init_with(const struct SlistParams params);

// clone a list
const struct Slist *slist_clone(const struct Slist* const from);

// free list
void slist_free(const struct Slist* const list);

// free iterator
void slist_it_free(const struct SlistIt* const it);

/*
 * Access
 */

// true if this list contains the specified element
bool slist_contains(const struct Slist* const list, const char* const val);

// put first index of val in index if present, 0 and return false if not present
bool slist_index_of(size_t *index, const struct Slist* const list, const char* const val);

// element at zero indexed position
const char *slist_at(const struct Slist* const list, const size_t i);

// find the first, NULL when no matches or allow_null_val and NULL present, first entry when empty filter
const char *slist_find(const struct Slist* const list, const struct SlistFilter filter);

// create an iterator at the start, caller must slist_it_free or invoke slist_next/prev until NULL
const struct SlistIt *slist_it_start(const struct Slist* const list);

// create an iterator at the end
const struct SlistIt *slist_it_end(const struct Slist* const list);

// create a filtering iterator, return NULL when no matches, first entry when empty filter
const struct SlistIt *slist_filter_it_start(const struct Slist* const list, const struct SlistFilter filter);

// create a filtering iterator at the end of the list, return NULL when no matches, last entry when empty filter
const struct SlistIt *slist_filter_it_end(const struct Slist* const list, const struct SlistFilter filter);

// next iterator val, NULL at end of list
const struct SlistIt *slist_it_next(const struct SlistIt* const it);

// prev iterator val, NULL at beginning of list
const struct SlistIt *slist_it_prev(const struct SlistIt* const it);

/*
 * Mutate
 */

// add at index, appends when index >= size, return true if added
bool slist_insert(const struct Slist* const list, size_t index, const char* const val);

// add to end, return true if added
bool slist_append(const struct Slist* const list, const char* const val);

// add to start, return true if added
bool slist_prepend(const struct Slist* const list, const char* const val);

// replace val at index, return true if replaced, NOP when index >= size
bool slist_replace(const struct Slist* const list, size_t index, const char* const val);

// add from vals, return number added
size_t slist_append_all(const struct Slist* const list, const struct Slist* const from);

// if the list contains val, remove the first occurrence and return true
bool slist_remove(const struct Slist* const list, const char* const val);

// remove val at i, return true if removed
bool slist_remove_at(const struct Slist* const list, const size_t i);

// remove all vals, returning number removed
size_t slist_remove_all(const struct Slist* const list);

// remove the it.val, return true if removed, it is unusable, slist_it_next or slist_it_prev must be called
bool slist_it_remove(const struct SlistIt* const it);

// shell sort in place
void slist_sort(const struct Slist* const list);

/*
 * Comparison
 */

// same length, vals equal
bool slist_equal(const struct Slist* const a, const struct Slist* const b);

// same length, vals equal in order
bool slist_equal_ordered(const struct Slist* const a, const struct Slist* const b);

/*
 * Info
 */

// to string, user frees, format "%s\n"
char *slist_str(const struct Slist* const list);

// number of values
size_t slist_size(const struct Slist* const list);

#endif // SLIST_H

