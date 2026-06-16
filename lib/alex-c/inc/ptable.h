#ifndef PTABLE_H
#define PTABLE_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * Array backed pointer indexed table.
 * Entries preserve insertion order.
 * Operations linearly traverse keys.
 * NULL values permitted.
 */
struct PTable; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct PTableIterState; // IWYU pragma: keep
struct PTableIter {
	const void *key;
	const void *val;
	struct PTableIterState *st;
};

// TODO consider fn_str
// TODO consider fn_free val
// TODO consider fn_alloc val for clone

/*
 * Optional constructor params (default)
 */
struct PTableParams {
	const fn_equal equal_key;   // _get, _put, _equal, _clone       (compare key pointers)
	const fn_equal equal_val;   // _equal                           (compare val pointers)
	const fn_alloc alloc_key;   // _clone, _put, must be idempotent (use key pointer)
	const fn_free free_key;     // _remove, _free, _free_vals       (NOP)
	const fn_free free_val;     // _free_vals                       (free)
	const fn_str str_key;       // _str                             (%p)
	const fn_str str_val;       // _str                             (%p)
	const size_t initial;       // initial capacity                 (10)
	const size_t grow;          // grow capacity by                 (10)
};

/*
 * Lifecycle
 */

// construct a table with PTableParams defaults
const struct PTable *ptable_init(void);

// construct a table with params
const struct PTable *ptable_init_with(const struct PTableParams params);

// clone a table, NULL clone_val for shallow clone
const struct PTable *ptable_clone(const struct PTable* const from, fn_clone clone_val);

// free table
void ptable_free(const struct PTable* const tab);

// free table and vals
void ptable_free_vals(const struct PTable* const tab);

// free iter
void ptable_iter_free(const struct PTableIter* const iter);

/*
 * Access
 */

// return val, NULL if not present
const void *ptable_get(const struct PTable* const tab, const void* const key);

// create an iterator, caller must ptable_iter_free or invoke ptable_next until NULL
const struct PTableIter *ptable_iter(const struct PTable* const tab);

// create an iterator filtering by equal_key and equal_val, NULL tests match all
const struct PTableIter *ptable_filter_iter(const struct PTable* const tab, fn_equal equal_key, fn_equal equal_val, const void* const data);

// next iterator entry, NULL at end of table
const struct PTableIter *ptable_iter_next(const struct PTableIter* const iter);

/*
 * Mutate
 */

// set key/val, return old val if overwritten
const void *ptable_put(const struct PTable* const tab, const void* const key, const void* const val);

// remove key, return old val if present
const void *ptable_remove(const struct PTable* const tab, const void* const key);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses equal_key and equal_val from a
bool ptable_equal(const struct PTable* const a, const struct PTable* const b);

/*
 * Conversion
 */

// ordered key pointers, caller frees list only
struct SList *ptable_keys_slist(const struct PTable* const tab);

// ordered val pointers, caller frees list only
struct SList *ptable_vals_slist(const struct PTable* const tab);

/*
 * Info
 */

// to string, user frees, format "str_key = str_val\n"
char *ptable_str(const struct PTable* const tab);

// number of entries
size_t ptable_size(const struct PTable* const tab);

#endif // PTABLE_H

