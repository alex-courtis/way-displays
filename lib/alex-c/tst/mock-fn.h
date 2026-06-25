#ifndef MOCK_FN
#define MOCK_FN

#include <stdbool.h>
#include <stddef.h>

bool mock_equal(const void* const a, const void* const b);

bool mock_less_than(const void* const a, const void* const b);

bool mock_match_val(const void* const val, const void* const data);

bool mock_match_key_val(const void* const key, const void* const val, const void* const data);

bool mock_match_smap(const char *key, const void* const val, const void* const data);

bool mock_match_smaps(const char *key, const char* const val, const void* const data);

bool mock_match_smapi(const char *key, const size_t val, const void* const data);

bool mock_match_sset(const char* const val, const void* const data);

bool mock_match_imap(const size_t key, const void* const val, const void* const data);

bool mock_test(const void* const val);

void *mock_clone(const void* const val);

void *mock_alloc(const void* const val);

void mock_free(const void* const val);

char* mock_str(const void* const val);

#endif // MOCK_FN
