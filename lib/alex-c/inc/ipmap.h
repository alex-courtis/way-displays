#ifndef IPMAP_H
#define IPMAP_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * `IPmap` with `size_t` keys
 */
struct IPmap; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct IPmapItState; // IWYU pragma: keep
struct IPmapIt {
	size_t key;
	const void *val;
	struct IPmapItState *st;
};

/*
 * Key/Val
 */
struct IPmapPair {
	size_t key;
	const void *val;
};

/*
 * Filter, must match all when multiple predicates specified, empty filter matches anything
 */
struct IPmapFilter {
	// test keys or vals
	fn_pred_szt key;
	fn_pred val;
	fn_2pred_szt key_val;

	// test keys or vals against user data
	const void *data;
	fn_2pred_szt key_data;
	fn_2pred val_data;
	fn_3pred_szt_ptr key_val_data;
};


/*
 * Optional constructor params (default)
 */
struct IPmapParams {
	const fn_equal equal_val;  // compare key pointers
	const fn_clone alloc_val;  // assign key pointer
	const fn_free free_val;    // free
	const fn_clone clone_val;  // NOP
	const fn_str str_val;      // %p
	const bool allow_null_val; // false
	const size_t initial;      // 10
	const size_t grow;         // 10
};

/*
 * Lifecycle
 */

// construct with IPmapParams defaults
const struct IPmap *ipmap_init(void);

// construct with params
const struct IPmap *ipmap_init_with(const struct IPmapParams params);

// same params, caller frees vals when alloc_val present [alloc_val]
const struct IPmap *ipmap_clone(const struct IPmap* const from);

// same params, caller frees vals, NULL on NULL clone_val, alloc_val overrides clone_val [alloc_val, clone_val]
const struct IPmap *ipmap_clone_deep(const struct IPmap* const from);

// free map
void ipmap_free(const struct IPmap* const map);

// free map and vals [free_val]
void ipmap_free_vals(const struct IPmap* const map);

// free iterator
void ipmap_it_free(const struct IPmapIt* const it);

/*
 * Access
 */

// return val, NULL if not present
const void *ipmap_get(const struct IPmap* const map, const size_t key);

// true if key is present
bool ipmap_contains_key(const struct IPmap* const map, const size_t key);

// true if val is present [equal_val]
bool ipmap_contains_val(const struct IPmap* const map, const void* const val);

// element at zero indexed position
struct IPmapPair ipmap_at(const struct IPmap* const map, const size_t i);

// find the first key/val pred, {NULL,NULL} when no matches, first when empty filter
struct IPmapPair ipmap_find(const struct IPmap* const map, const struct IPmapFilter filter);

// create an iterator, caller must ipmap_it_free or invoke ipmap_next until NULL
const struct IPmapIt *ipmap_it(const struct IPmap* const map);

// create a filtering iterator, return NULL when no matches, caller must ipmap_it_free or invoke ipmap_next until NULL
const struct IPmapIt *ipmap_filter_it(const struct IPmap* const map, const struct IPmapFilter filter);

// next iterator entry, NULL at end of map
const struct IPmapIt *ipmap_it_next(const struct IPmapIt* const it);

/*
 * Mutate
 */

// set key/val, return old val if overwritten [alloc_val]
const void *ipmap_put(const struct IPmap* const map, const size_t key, const void* const val);

// set key/val if not present, return existing val if present [alloc_val]
const void *ipmap_put_if_absent(const struct IPmap* const map, const size_t key, const void* const val);

// set key/val, free old val, return true if overwritten [alloc_val, free_val]
bool ipmap_put_free(const struct IPmap* const map, const size_t key, const void* const val);

// set all from key/val, returning number overwritten [alloc_val]
size_t ipmap_put_all(const struct IPmap* const map, const struct IPmap* const from);

// set all from key/val, returning number overwritten, freeing overwritten vals [alloc_val, free_val]
size_t ipmap_put_all_free(const struct IPmap* const map, const struct IPmap* const from);

// set all from key/val, returning number overwritten, NOP when NULL clone_val  [clone_val]
size_t ipmap_put_all_clone(const struct IPmap* const map, const struct IPmap* const from);

// set all from key/val, returning number overwritten, freeing overwritten vals, NOP when NULL clone_val [free_val, clone_val]
size_t ipmap_put_all_clone_free(const struct IPmap* const map, const struct IPmap* const from);

// remove entry, if removed return old val
const void *ipmap_remove(const struct IPmap* const map, const size_t key);

// remove and free entry, if removed free it and return true [free_val]
bool ipmap_remove_free(const struct IPmap* const map, const size_t key);

// remove all entries, returning number removed
size_t ipmap_remove_all(const struct IPmap* const map);

// remove all entries and free, returning number removed [free_val]
size_t ipmap_remove_all_free(const struct IPmap* const map);

// remove entries matching from keys, return number removed
size_t ipmap_remove_from(const struct IPmap* const map, const struct IPmap* const from);

// remove and free entries matching from keys, return number removed [free_val]
size_t ipmap_remove_from_free(const struct IPmap* const map, const struct IPmap* const from);

// remove the entry, it is unusable, ipmap_it_next must be called
void ipmap_it_remove(const struct IPmapIt* const it);

// remove and entry, free the val, it is unusable, ipmap_it_next must be called [free_val]
void ipmap_it_remove_free(const struct IPmapIt* const it);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses params from a [equal_val]
bool ipmap_equal(const struct IPmap* const a, const struct IPmap* const b);

/*
 * Conversion
 */

// map ordered vals, caller frees list, caller frees contents when alloc_val present [alloc_val]
struct Pslist *ipmap_vals_pslist(const struct IPmap* const map);

// map ordered vals, caller frees list and vals, NULL when NULL clone_val [clone_val]
struct Pslist *ipmap_vals_pslist_clone(const struct IPmap* const map);

// map ordered vals, same params, caller frees set, caller frees vals when alloc_val present [alloc_val]
const struct Pset *ipmap_vals_pset(const struct IPmap* const map);

// map ordered vals, same params, caller frees set and vals, NULL on NULL clone_val or both alloc_val and clone_val [clone_val]
const struct Pset *ipmap_vals_pset_clone(const struct IPmap* const map);

/*
 * Info
 */

// to string, user frees, format "k = str_val\n"
char *ipmap_str(const struct IPmap* const map);

// number of entries
size_t ipmap_size(const struct IPmap* const map);

#endif // IPMAP_H

