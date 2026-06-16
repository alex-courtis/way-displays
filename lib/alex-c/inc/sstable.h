#ifndef SSTABLE_H
#define SSTABLE_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * `STable` with string vals.
 * Values are strdup'd on successful `sstable_put`, `sstable_clone` and `sstable_vals_slist`
 * Values are free'd on `sset_free`
 */
struct SSTable; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct SSTableIterState; // IWYU pragma: keep
struct SSTableIter {
	const char *key;
	const char *val;
	struct SSTableIterState *st;
};

/*
 * Optional constructor params (default)
 */
struct SSTableParams {
	const bool case_insensitive; // keys             (false)
	const size_t initial;        // initial capacity (10)
	const size_t grow;           // grow capacity by (10)
};

/*
 * Lifecycle
 */

// construct a table with SSTableParams defaults
const struct SSTable *sstable_init(void);

// construct a table with params
const struct SSTable *sstable_init_with(const struct SSTableParams params);

// clone a table
const struct SSTable *sstable_clone(const struct SSTable* const from);

// free table keys and values
void sstable_free(const struct SSTable* const tab);

// free iter
void sstable_iter_free(const struct SSTableIter* const iter);

/*
 * Access
 */

// return val, NULL if not present
const void *sstable_get(const struct SSTable* const tab, const char* const key);

// create an iterator, caller must sstable_iter_free or invoke sstable_next until NULL
const struct SSTableIter *sstable_iter(const struct SSTable* const tab);

// create an iterator filtering by equal_key and equal_val, NULL tests match all
const struct SSTableIter *sstable_filter_iter(const struct SSTable* const tab, fn_equal_str equal_key, fn_equal_str equal_val, const void* const data);

// next iterator entry, NULL at end of table
const struct SSTableIter *sstable_iter_next(const struct SSTableIter* const iter);

/*
 * Mutate
 */

// set key/val, return true if overwritten
bool sstable_put(const struct SSTable* const tab, const char* const key, const char* const val);

// remove key, return true if removed
bool sstable_remove(const struct SSTable* const tab, const char* const key);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses case from a
bool sstable_equal(const struct SSTable* const a, const struct SSTable* const b);

/*
 * Conversion
 */

// ordered key strings, caller frees list and vals
struct SList *sstable_keys_slist(const struct SSTable* const tab);

// ordered key strings, caller frees list and vals
struct SList *sstable_vals_slist(const struct SSTable* const tab);

/*
 * Info
 */

// to string, user frees, format "k = v\n"
char *sstable_str(const struct SSTable* const tab);

// number of entries
size_t sstable_size(const struct SSTable* const tab);

#endif // SSTABLE_H

