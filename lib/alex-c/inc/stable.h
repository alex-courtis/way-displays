#ifndef STABLE_H
#define STABLE_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * Array backed string indexed table.
 * Entries preserve insertion order.
 * Operations linearly traverse keys.
 * NULL values permitted.
 * Not thread safe.
 */
struct STable; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct STableIter; // IWYU pragma: keep

/*
 * Lifecycle
 */

// construct a table with initial size 10, growing by 10 as necessary, case sensitive
const struct STable *stable_init(void);

// construct a table with initial size, growing as necessary, NULL on zero param
const struct STable *stable_init_with(const size_t initial, const size_t grow, const bool case_insensitive);

// free table
void stable_free(const void* const tab);

// free table and vals, null fn_free uses free()
void stable_free_vals(const struct STable* const tab, fn_free);

// free iter
void stable_iter_free(const struct STableIter* const iter);

/*
 * Access
 */

// return val, NULL if not present
const void *stable_get(const struct STable* const tab, const char* const key);

// create an iterator, caller must stable_iter_free or invoke stable_next until NULL
const struct STableIter *stable_iter(const struct STable* const tab);

// next iterator entry, NULL at end of table
const struct STableIter *stable_iter_next(const struct STableIter* const iter);

// iterator key, NULL on NULL iter
const char *stable_iter_key(const struct STableIter* const iter);

// iterator value, NULL on NULL iter
const void *stable_iter_val(const struct STableIter* const iter);

/*
 * Mutate
 */

// set key/val, return old val if overwritten
const void *stable_put(const struct STable* const tab, const char* const key, const void* const val);

// remove key, return old val if present
const void *stable_remove(const struct STable* const tab, const char* const key);

/*
 * Comparison
 */

// same length, keys and vals equal, case sensitivity is from a, NULL equal compares pointers
bool stable_equal(const struct STable* const a, const struct STable* const b, fn_equal);

/*
 * Conversion
 */

// ordered key pointers to table, caller frees list and contents
struct SList *stable_keys_slist(const struct STable* const tab);

// ordered val pointers to table, caller frees list only
struct SList *stable_vals_slist(const struct STable* const tab);

/*
 * Info
 */

// to string, user frees
// lines with format "%s = %s\n"
// NULL vals printed as "(null)"
// fn_str NULL for char* vals
char *stable_str(const struct STable* const tab, fn_str);

// number of entries
size_t stable_size(const struct STable* const tab);

// current capacity: initial + n * grow
size_t stable_capacity(const struct STable* const tab);

#endif // STABLE_H

