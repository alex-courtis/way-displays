#ifndef STABLE_H
#define STABLE_H

#include <stddef.h>

/*
 * Array backed string indexed table.
 * Entries preserve insertion order.
 * Operations linearly traverse keys.
 * NULL values permitted.
 * Not thread safe.
 */
struct STable;

/*
 * Entry iterator.
 */
struct STableIter {
	const char* const key;
	const void* const val;
};

/*
 * Lifecycle
 */

// construct a table with initial size, growing as necessary, NULL on zero param
const struct STable *stable_init(const size_t initial, const size_t grow);

// free table
void stable_free(const void* const tab);

// free table and vals, null free_val uses free()
void stable_free_vals(const struct STable* const tab, void (*free_val)(const void* const val));

// free iter
void stable_iter_free(const struct STableIter* const iter);

/*
 * Access
 */

// return val, NULL not present
const void *stable_get(const struct STable* const tab, const char* const key);

// create an iterator, caller must stable_iter_free or invoke stable_next until NULL
const struct STableIter *stable_iter(const struct STable* const tab);

// next iterator value, NULL at end of list
const struct STableIter *stable_next(const struct STableIter* const iter);

// number of entries with val
size_t stable_size(const struct STable* const tab);

/*
 * Mutate
 */

// set key/val, return old val if overwritten
const void *stable_put(const struct STable* const tab, const char* const key, const void* const val);

// remove key, return old val if present
const void *stable_remove(const struct STable* const tab, const char* const key);

#endif // STABLE_H

