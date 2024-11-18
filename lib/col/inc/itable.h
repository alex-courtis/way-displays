#ifndef ITABLE_H
#define ITABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "fn.h"

/*
 * Array backed integer indexed table.
 * Entries preserve insertion order.
 * Operations linearly traverse keys.
 * NULL values permitted.
 * Not thread safe.
 */
struct ITable;

/*
 * Entry iterator.
 */
struct ITableIter {
	const uint64_t key;
	const void* const val;
};

/*
 * Lifecycle
 */

// construct a table with initial size, growing as necessary, NULL on zero param
const struct ITable *itable_init(const size_t initial, const size_t grow);

// free table
void itable_free(const void* const tab);

// free table and vals, null fn_free_val uses free()
void itable_free_vals(const struct ITable* const tab, fn_free_val);

// free iter
void itable_iter_free(const struct ITableIter* const iter);

/*
 * Access
 */

// return val, NULL not present
const void *itable_get(const struct ITable* const tab, const uint64_t key);

// create an iterator, caller must itable_iter_free or invoke itable_next until NULL
const struct ITableIter *itable_iter(const struct ITable* const tab);

// next iterator value, NULL at end of list
const struct ITableIter *itable_next(const struct ITableIter* const iter);

/*
 * Mutate
 */

// set key/val, return old val if overwritten
const void *itable_put(const struct ITable* const tab, const uint64_t key, const void* const val);

// remove key, return old val if present
const void *itable_remove(const struct ITable* const tab, const uint64_t key);

/*
 * Comparison
 */

// same length, keys and vals equal in order, NULL equal compares pointers
bool itable_equal(const struct ITable* const a, const struct ITable* const b, fn_equals);

/*
 * Conversion
 */

// ordered uint64_t* key pointers to table, caller frees list only
struct SList *itable_keys_slist(const struct ITable* const tab);

// ordered val pointers to table, caller frees list only
struct SList *itable_vals_slist(const struct ITable* const tab);

/*
 * Info
 */

// number of entries with val
size_t itable_size(const struct ITable* const tab);

// current capacity: initial + n * grow
size_t itable_capacity(const struct ITable* const tab);

#endif // ITABLE_H

