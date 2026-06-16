#ifndef MOCK_FN
#define MOCK_FN

#include <stdbool.h>
#include <stddef.h>

bool mock_equal(const void* const a, const void* const b);

bool mock_equal_str(const char* const a, const char* const b);

bool mock_equal_size_t(const size_t a, const void* const b);

bool mock_less_than(const void* const a, const void* const b);

bool mock_test(const void* const data);

const void *mock_alloc(const void* const val);

void mock_free(const void* const val);

void *mock_clone(const void* const val);

char* mock_str(const void* const val);

#endif // MOCK_FN
