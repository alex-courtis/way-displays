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
typedef bool (*fn_pred_p)(const void* const p);

typedef bool (*fn_pred_s)(const char* const s);

typedef bool (*fn_pred_i)(const size_t i);

//
// bi-predicate
//
typedef bool (*fn_pred_p_p)(const void* const p1, const void* const p2);

typedef bool (*fn_pred_s_p)(const char* const s,  const void* const p);

typedef bool (*fn_pred_s_s)(const char* const s1, const char* const s2);

typedef bool (*fn_pred_s_i)(const char* const s,  const size_t i);

typedef bool (*fn_pred_i_p)(const size_t i,       const void* const p);

//
// tri-predicate
//
typedef bool (*fn_pred_p_p_p)(const void* const p1, const void* const p2, const void* const p3);

typedef bool (*fn_pred_s_p_p)(const char* const s,  const void* const p1, const void* const p2);

typedef bool (*fn_pred_s_s_p)(const char* const s1, const char* const s2, const void* const p);

typedef bool (*fn_pred_s_i_p)(const char* const s,  const size_t i,       const void* const p);

typedef bool (*fn_pred_i_p_p)(const size_t i,       const void* const p1, const void* const p2);

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
