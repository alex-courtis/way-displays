#ifndef MOCK_FN
#define MOCK_FN

#include <stdbool.h>
#include <stddef.h>

bool mock_equal(const void* const a, const void* const b);

bool mock_less_than(const void* const a, const void* const b);

bool mock_pred     (const void* const ptr);

bool mock_2pred    (const void* const ptr, const void* const data);

bool mock_2pred_str(const char* const str, const void* const data);

bool mock_2pred_szt(const size_t i,        const void* const data);

bool mock_3pred        (const void* const ptr1, const void* const ptr2, const void* const data);

bool mock_3pred_str_ptr(const char* const str,  const void* const ptr,  const void* const data);

bool mock_3pred_str_str(const char* const str1, const char* const str2, const void* const data);

bool mock_3pred_str_szt(const char* const str,  const size_t i,         const void* const data);

bool mock_3pred_szt_ptr(const size_t i,         const void* const ptr,  const void* const data);

void *mock_clone(const void* const ptr);

void *mock_alloc(const void* const ptr);

void mock_free(void *ptr);

char* mock_str(const void* const ptr);

#endif // MOCK_FN
