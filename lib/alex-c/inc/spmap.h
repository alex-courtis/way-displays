#ifndef SPMAP_H
#define SPMAP_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * `PPmap` with string keys.
 * Keys are memory managed.
 */
struct SPmap; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct SPmapItState; // IWYU pragma: keep
struct SPmapIt {
	const char *key;
	const void *val;
	struct SPmapItState *st;
};

/*
 * Optional constructor params (default)
 */
struct SPmapParams {
	const bool case_insensitive; // false
	const fn_equal equal_val;    // compare key pointers
	const fn_clone alloc_val;    // assign val pointer
	const fn_free free_val;      // free
	const fn_clone clone_val;    // NOP
	const fn_str str_val;        // %p
	const bool allow_null_val;   // false
	const size_t initial;        // 10
	const size_t grow;           // 10
};

/*
 * Key/Val
 */
struct SPmapPair {
	const char *key;
	const void *val;
};

/*
 * Lifecycle
 */

// construct with SPmapParams defaults
const struct SPmap *spmap_init(void);

// construct with params
const struct SPmap *spmap_init_with(const struct SPmapParams params);

// same params, caller frees vals when alloc_val present [alloc_val]
const struct SPmap *spmap_clone(const struct SPmap* const from);

// same params, caller frees vals, NULL on NULL clone_val, alloc_val overrides clone_val [alloc_key, alloc_val, clone_val]
const struct SPmap *spmap_clone_deep(const struct SPmap* const from);

// free map
void spmap_free(const struct SPmap* const map);

// free map and vals [free_val]
void spmap_free_vals(const struct SPmap* const map);

// free iterator
void spmap_it_free(const struct SPmapIt* const it);

/*
 * Access
 */

// return val, NULL if not present
const void *spmap_get(const struct SPmap* const map, const char* const key);

// true if key is present
bool spmap_contains_key(const struct SPmap* const map, const char* const key);

// true if val is present [equal_val]
bool spmap_contains_val(const struct SPmap* const map, const void* const val);

// find the first key/val match, {NULL,NULL} when no matches or NULL match
struct SPmapPair spmap_match(const struct SPmap* const map, fn_3pred_str_ptr match, const void* const data);

// find the first key match, {NULL,NULL} when no matches or NULL match
struct SPmapPair spmap_match_key(const struct SPmap* const map, fn_2pred_str match, const void* const data);

// find the first val match, {NULL,NULL} when no matches or NULL match
struct SPmapPair spmap_match_val(const struct SPmap* const map, fn_2pred match, const void* const data);

// create an iterator, caller must spmap_it_free or invoke spmap_next until NULL
const struct SPmapIt *spmap_it(const struct SPmap* const map);

// create an iterator filtering by key/val match, return NULL when no matches or NULL match
const struct SPmapIt *spmap_match_it(const struct SPmap* const map, fn_3pred_str_ptr match, const void* const data);

// create an iterator filtering by key match, return NULL when no matches or NULL match
const struct SPmapIt *spmap_match_key_it(const struct SPmap* const map, fn_2pred_str match, const void* const data);

// create an iterator filtering by val match, return NULL when no matches or NULL match
const struct SPmapIt *spmap_match_val_it(const struct SPmap* const map, fn_2pred match, const void* const data);

// next iterator entry, NULL at end of map
const struct SPmapIt *spmap_it_next(const struct SPmapIt* const it);

/*
 * Mutate
 */

// set key/val, return old val if overwritten [alloc_val]
const void *spmap_put(const struct SPmap* const map, const char* const key, const void* const val);

// set key/val if not present, return existing val if present [alloc_val]
const void *spmap_put_if_absent(const struct SPmap* const map, const char* const key, const void* const val);

// set key/val, free old val, return true if overwritten [alloc_val, free_val]
bool spmap_put_free(const struct SPmap* const map, const  char* const key, const void* const val);

// set all from key/val, returning number overwritten [alloc_val]
size_t spmap_put_all(const struct SPmap* const map, const struct SPmap* const from);

// set all from key/val, returning number overwritten, freeing overwritten vals [alloc_val, free_val]
size_t spmap_put_all_free(const struct SPmap* const map, const struct SPmap* const from);

// set all from key/val, returning number overwritten, NOP when NULL clone_val  [clone_val]
size_t spmap_put_all_clone(const struct SPmap* const map, const struct SPmap* const from);

// set all from key/val, returning number overwritten, freeing overwritten vals, NOP when NULL clone_val [free_val, clone_val]
size_t spmap_put_all_clone_free(const struct SPmap* const map, const struct SPmap* const from);

// remove entry, if removed return old val
const void *spmap_remove(const struct SPmap* const map, const char* const key);

// remove and free entry, if removed free it and return true [free_val]
bool spmap_remove_free(const struct SPmap* const map, const char* const key);

// remove entries matching from keys, return number removed [equal_key]
size_t spmap_remove_all(const struct SPmap* const map, const struct SPmap* const from);

// remove and free entries matching from keys, return number removed [equal_key, free_val]
size_t spmap_remove_all_free(const struct SPmap* const map, const struct SPmap* const from);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses params from a [equal_val]
bool spmap_equal(const struct SPmap* const a, const struct SPmap* const b);

/*
 * Conversion
 */

// map ordered keys, caller frees list and contents
struct Pslist *spmap_keys_pslist(const struct SPmap* const map);

// map ordered keys, same params
const struct Sset *spmap_keys_sset(const struct SPmap* const map);

// map ordered vals, caller frees list, caller frees contents when alloc_val present [alloc_val]
struct Pslist *spmap_vals_pslist(const struct SPmap* const map);

// map ordered vals, caller frees list and vals, NULL when NULL clone_val [clone_val]
struct Pslist *spmap_vals_pslist_clone(const struct SPmap* const map);

// map ordered vals, same params, caller frees set, caller frees vals when alloc_val present [alloc_val]
const struct Pset *spmap_vals_pset(const struct SPmap* const map);

// map ordered vals, same params, caller frees set and vals, NULL on NULL clone_val or both alloc_val and clone_val [clone_val]
const struct Pset *spmap_vals_pset_clone(const struct SPmap* const map);


/*
 * Info
 */

// to string, user frees, format "k = str_val\n"
char *spmap_str(const struct SPmap* const map);

// number of entries
size_t spmap_size(const struct SPmap* const map);

#endif // SPMAP_H

