#ifndef FN_H
#define FN_H

#include <stdbool.h>

//
// a is generally the value from the collection, b is user supplied
//
typedef bool (*fn_equals)(const void* const a, const void* const b);

// true if both NULL or strcmp(a, b) == 0
bool fn_comp_equals_strcmp(const void* const a, const void* const b);

// true if both NULL or strstr(a, b)
bool fn_comp_equals_strstr(const void* const a, const void* const b);

//
// a < b
//
typedef bool (*fn_less_than)(const void* const a, const void* const b);

//
// arbitrary test
//
typedef bool (*fn_test)(const void* const val);

//
// free
//
typedef void (*fn_free_val)(const void* const val);

#endif // FN_H

