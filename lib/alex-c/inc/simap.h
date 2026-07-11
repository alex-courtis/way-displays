#ifndef SIMAP_H
#define SIMAP_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * `PPmap` with string keys and size_t vals.
 * Keys are memory managed.
 */
struct SImap; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct SImapItState; // IWYU pragma: keep
struct SImapIt {
	const char *key;
	size_t val;
	struct SImapItState *st;
};

/*
 * Optional constructor params (default)
 */
struct SImapParams {
	const bool case_insensitive_key; // (false)
	const size_t initial;            // (10)
	const size_t grow;               // (10)
};

/*
 * Key/Val
 */
struct SImapPair {
	const char *key;
	size_t val;
};

/*
 * Lifecycle
 */

// construct with SImapParams defaults
const struct SImap *simap_init(void);

// construct with params
const struct SImap *simap_init_with(const struct SImapParams params);

// same params
const struct SImap *simap_clone(const struct SImap* const from);

// free map
void simap_free(const struct SImap* const map);

// free iterator
void simap_it_free(const struct SImapIt* const it);

/*
 * Access
 */

// return val, will return 0 if not present
size_t simap_get(const struct SImap* const map, const char* const key);

// populate np with val if present, 0 and return false if not present
bool simap_get_ptr(size_t* np, const struct SImap* const map, const char* const key);

// true if key is present
bool simap_contains_key(const struct SImap* const map, const char* const key);

// true if val is present
bool simap_contains_val(const struct SImap* const map, const size_t val);

// find the first key/val match, {NULL,0} when no matches or NULL match
struct SImapPair simap_match(const struct SImap* const map, fn_3pred_str_szt match, const void* const data);

// find the first key match, {NULL,0} when no matches or NULL match
struct SImapPair simap_match_key(const struct SImap* const map, fn_2pred_str match, const void* const data);

// find the first val match, {NULL,0} when no matches or NULL match
struct SImapPair simap_match_val(const struct SImap* const map, fn_2pred_szt match, const void* const data);

// create an iterator, caller must simap_it_free or invoke simap_next until NULL
const struct SImapIt *simap_it(const struct SImap* const map);

// create an iterator filtering by key/val match, return NULL when no matches or NULL match
const struct SImapIt *simap_match_it(const struct SImap* const map, fn_3pred_str_szt match, const void* const data);

// create an iterator filtering by key match, return NULL when no matches or NULL match
const struct SImapIt *simap_match_key_it(const struct SImap* const map, fn_2pred_str match, const void* const data);

// create an iterator filtering by val match, return NULL when no matches or NULL match
const struct SImapIt *simap_match_val_it(const struct SImap* const map, fn_2pred_szt match, const void* const data);

// next iterator entry, NULL at end of map
const struct SImapIt *simap_it_next(const struct SImapIt* const it);

/*
 * Mutate
 */

// set key/val, return true if overwritten
bool simap_put(const struct SImap* const map, const char* const key, const size_t val);

// set key/val if not present, return true if overwritten
bool simap_put_if_absent(const struct SImap* const map, const char* const key, const size_t val);

// set all from key/val, returning number of overwritten
size_t simap_put_all(const struct SImap* const map, const struct SImap* const from);

// remove entry, if removed return true
bool simap_remove(const struct SImap* const map, const char* const key);

// remove entries matching from keys, return number removed
size_t simap_remove_all(const struct SImap* const map, const struct SImap* const from);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses case from a
bool simap_equal(const struct SImap* const a, const struct SImap* const b);

/*
 * Conversion
 */

// map ordered keys, caller frees list and contents
struct Pslist *simap_keys_pslist(const struct SImap* const map);

// map ordered keys, same params
const struct Sset *simap_keys_sset(const struct SImap* const map);

/*
 * Info
 */

// to string, user frees, format "k = v\n"
char *simap_str(const struct SImap* const map);

// number of entries
size_t simap_size(const struct SImap* const map);

#endif // SIMAP_H

