#ifndef FN_H
#define FN_H

#include <stdbool.h>

// TODO remove the fn_ prefix from implementations

//
// a is generally the value from the collection, b generally user supplied or the value from the other collection
//
typedef bool (*fn_equal)(const void* const a, const void* const b);

// true if a == b
bool fn_equal_ptr(const void* const a, const void* const b);

// true if both NULL or strcmp(a, b) == 0
bool fn_equal_strcmp(const char* const a, const char* const b);

// true if both NULL or strcasecmp(a, b) == 0
bool fn_equal_strcasecmp(const char* const a, const char* const b);

// true if both NULL or strstr(a, b)
bool fn_equal_strstr(const char* const a, const char* const b);

//
// a < b
//
typedef bool (*fn_less_than)(const void* const a, const void* const b);

// strcmp(a, b) <= 0
bool fn_less_than_strcmp(const char* const a, const char* const b);

// strcasecmp(a, b) < 0
bool fn_less_than_strcasecmp(const char* const a, const char* const b);

//
// match against supplied data
//
typedef bool (*fn_match_val)(const void* const val, const void* const data);

typedef bool (*fn_match_key_val)(const void* const key, const void* const val, const void* const data);

//
// arbitrary test
//
typedef bool (*fn_test)(const void* const data);

//
// free
//
typedef void (*fn_free)(const void* const val);

//
// clone
//
typedef void* (*fn_clone)(const void* const val);

// copies a string using strdup, if val is NULL, returns NULL
void *fn_clone_strdup(const void* const val);

//
// allocate a new val, must equal original
//
typedef void* (*fn_alloc)(const void* const val);

//
// to string, caller frees, may return NULL
//
typedef char* (*fn_str)(const void* const val);

// val or "(null)"
char *fn_str_or_null(const char* const val);

#endif // FN_H
