#ifndef PTABLE_H
#define PTABLE_H

#include <stddef.h>

/*
 * ITable convenience wrapper with pointer key.
 */
struct PTable;

/*
 * Entry iterator.
 */
struct PTableIter {
	const void *key;
	const void *val;
};

/*
 * Lifecycle
 */

// construct a table with initial size, growing as necessary, NULL on zero param
const struct PTable *ptable_init(const size_t initial, const size_t grow);

// free table
void ptable_free(const void* const tab);

// free table and vals, null free_val uses free()
void ptable_free_vals(const struct PTable* const tab, void (*free_val)(const void* const val));

// free iter
void ptable_iter_free(const struct PTableIter* const iter);

/*
 * Access
 */

// return val, NULL not present
const void *ptable_get(const struct PTable* const tab, const void* const key);

// create an iterator, caller must ptable_iter_free or invoke ptable_next until NULL
const struct PTableIter *ptable_iter(const struct PTable* const tab);

// next iterator value, NULL at end of list
const struct PTableIter *ptable_next(const struct PTableIter* const iter);

// number of entries with val
size_t ptable_size(const struct PTable* const tab);

/*
 * Mutate
 */

// set key/val, return old val if overwritten, NULL val to remove
const void *ptable_put(const struct PTable* const tab, const void* const key, const void* const val);

#endif // PTABLE_H

