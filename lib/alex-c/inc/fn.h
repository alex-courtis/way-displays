#ifndef FN_H
#define FN_H

#include <stdbool.h>
#include <stddef.h>

//
// a equals b, a is generally the value from the collection, b generally user data or the value from the other collection
//
typedef bool (*fn_equal)(const void* const a, const void* const b);

// true if a == b
bool equal_ptr(const void* const a, const void* const b);

// true if both NULL or strcmp(a, b) == 0
bool equal_strcmp(const char* const a, const char* const b);

// true if both NULL or strcasecmp(a, b) == 0
bool equal_strcasecmp(const char* const a, const char* const b);

// true if both NULL or strstr(a, b)
bool equal_strstr(const char* const a, const char* const b);

// true if both present and equal value
bool equal_stp(const size_t* const a, const size_t* const b);

//
// a less than b, a is generally the value from the collection, b generally user data or the value from the other collection
//
typedef bool (*fn_less_than)(const void* const a, const void* const b);

// strcmp(a, b) <= 0
bool less_than_strcmp(const char* const a, const char* const b);

// strcasecmp(a, b) < 0
bool less_than_strcasecmp(const char* const a, const char* const b);

//
// predicate
//
typedef bool (*fn_pred)(const void* const ptr);

typedef bool (*fn_pred_str)(const char* const str);

typedef bool (*fn_pred_szt)(const size_t i);

// TODO rename params

//
// bi-predicate against user data
//
typedef bool (*fn_2pred)    (const void* const ptr, const void* const data);

typedef bool (*fn_2pred_str)(const char* const str, const void* const data);

typedef bool (*fn_2pred_str_str)(const char* const str, const char* const str2);

typedef bool (*fn_2pred_str_szt)(const char* const str, const size_t i);

typedef bool (*fn_2pred_szt)(const size_t i,        const void* const data);

//
// tri-predicate against user data, generally map key/val
//
typedef bool (*fn_3pred)        (const void* const ptr1, const void* const ptr2, const void* const data);

typedef bool (*fn_3pred_str_ptr)(const char* const str,  const void* const ptr,  const void* const data);

typedef bool (*fn_3pred_str_str)(const char* const str1, const char* const str2, const void* const data);

typedef bool (*fn_3pred_str_szt)(const char* const str,  const size_t i,         const void* const data);

typedef bool (*fn_3pred_szt_ptr)(const size_t i,         const void* const ptr,  const void* const data);

//
// free
//
typedef void (*fn_free)(void *ptr);

//
// clone
//
typedef void* (*fn_clone)(const void* const ptr);

// copies a string using strdup, return NULL on NULL str
void *clone_strdup(const char* const str);

// allocates and sets a size_t*, return NULL on NULL np
void *clone_size_t_ptr(const size_t* const np);

//
// to string, caller frees, may return NULL
//
typedef char* (*fn_str)(const void* const ptr);

// val or "(null)"
char *str_or_null(const char* const str);

// %zu or "(null)"
char *str_size_t_ptr(const size_t* const pi);

#endif // FN_H
