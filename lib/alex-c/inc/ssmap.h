#ifndef SSMAP_H
#define SSMAP_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * `PPmap` with string keys and vals.
 * Keys and values are memory managed.
 */
struct SSmap; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct SSmapItState; // IWYU pragma: keep
struct SSmapIt {
	const char *key;
	const char *val;
	struct SSmapItState *st;
};

/*
 * Optional constructor params (default)
 */
struct SSmapParams {
	const bool case_insensitive_key; // (false)
	const bool case_insensitive_val; // (false)
	const bool allow_null_val;       // false
	const size_t initial;            // (10)
	const size_t grow;               // (10)
};

/*
 * Key/Val
 */
struct SSmapPair {
	const char *key;
	const char *val;
};

/*
 * Lifecycle
 */

// construct with SSmapParams defaults
const struct SSmap *ssmap_init(void);

// construct with params
const struct SSmap *ssmap_init_with(const struct SSmapParams params);

// same params
const struct SSmap *ssmap_clone(const struct SSmap* const from);

// free map
void ssmap_free(const struct SSmap* const map);

// free iterator
void ssmap_it_free(const struct SSmapIt* const it);

/*
 * Access
 */

// return val, NULL if not present
const char *ssmap_get(const struct SSmap* const map, const char* const key);

// true if key is present
bool ssmap_contains_key(const struct SSmap* const map, const char* const key);

// true if val is present
bool ssmap_contains_val(const struct SSmap* const map, const char* const val);

// element at zero indexed position
struct SSmapPair ssmap_at(const struct SSmap* const map, const size_t i);

// find the first key/val pred, {NULL,NULL} when no matches or NULL match
struct SSmapPair ssmap_find(const struct SSmap* const map, fn_3pred_str_str pred_key_val, const void* const data);

// find the first key pred, {NULL,NULL} when no matches or NULL match
struct SSmapPair ssmap_find_key(const struct SSmap* const map, fn_2pred_str pred_key, const void* const data);

// find the first val pred, {NULL,NULL} when no matches or NULL match
struct SSmapPair ssmap_find_val(const struct SSmap* const map, fn_2pred_str pred_val, const void* const data);

// create an iterator, caller must ssmap_it_free or invoke ssmap_next until NULL
const struct SSmapIt *ssmap_it(const struct SSmap* const map);

// create an iterator filtering by key/val pred, return NULL when no matches or NULL match
const struct SSmapIt *ssmap_filter_it(const struct SSmap* const map, fn_3pred_str_str pred_key_val, const void* const data);

// create an iterator filtering by key pred, return NULL when no matches or NULL match
const struct SSmapIt *ssmap_key_filter_it(const struct SSmap* const map, fn_2pred_str pred_key, const void* const data);

// create an iterator filtering by val pred, return NULL when no matches or NULL match
const struct SSmapIt *ssmap_val_filter_it(const struct SSmap* const map, fn_2pred_str pred_val, const void* const data);

// next iterator entry, NULL at end of map
const struct SSmapIt *ssmap_it_next(const struct SSmapIt* const it);

/*
 * Mutate
 */

// set key/val, return true if overwritten
bool ssmap_put(const struct SSmap* const map, const char* const key, const char* const val);

// set key/val if not present, return true if overwritten
bool ssmap_put_if_absent(const struct SSmap* const map, const char* const key, const char* const val);

// set all from key/val, returning number of overwritten
size_t ssmap_put_all(const struct SSmap* const map, const struct SSmap* const from);

// remove entry, if removed return true
bool ssmap_remove(const struct SSmap* const map, const char* const key);

// remove entries matching from keys, return number removed
size_t ssmap_remove_all(const struct SSmap* const map, const struct SSmap* const from);

// remove the entry, it is unusable, ssmap_it_next must be called
void ssmap_it_remove(const struct SSmapIt* const it);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses case from a
bool ssmap_equal(const struct SSmap* const a, const struct SSmap* const b);

/*
 * Conversion
 */

// map ordered keys, caller frees list and contents
struct Pslist *ssmap_keys_pslist(const struct SSmap* const map);

// map ordered keys, same params
const struct Sset *ssmap_keys_sset(const struct SSmap* const map);

// map ordered vals, caller frees list and contents
struct Pslist *ssmap_vals_pslist(const struct SSmap* const map);

// map ordered vals, same params
const struct Sset *ssmap_vals_sset(const struct SSmap* const map);

/*
 * Info
 */

// to string, user frees, format "k = v\n"
char *ssmap_str(const struct SSmap* const map);

// number of entries
size_t ssmap_size(const struct SSmap* const map);

#endif // SSMAP_H

