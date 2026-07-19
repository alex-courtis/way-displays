#ifndef MOCK_FN
#define MOCK_FN

#include <stdbool.h>
#include <stddef.h>

bool mock_equal(const void* const a, const void* const b);


bool mock_less_than(const void* const a, const void* const b);


bool mock_pred_p(const void* const p);

bool mock_pred_s(const char* const s);

bool mock_pred_i(const size_t i);


bool mock_pred_p_p(const void* const p1, const void* const p2);

bool mock_pred_s_p(const char* const s,  const void* const p);

bool mock_pred_s_s(const char* const s1, const void* const s2);

bool mock_pred_s_i(const char* const s,  const size_t i);

bool mock_pred_i_p(const size_t i,       const void* const p);


bool mock_pred_p_p_p(const void* const p1, const void* const p2, const void* const p3);

bool mock_pred_s_p_p(const char* const s,  const void* const p1, const void* const p2);

bool mock_pred_s_s_p(const char* const s1, const char* const s2, const void* const p);

bool mock_pred_s_i_p(const char* const s,  const size_t i,       const void* const p);

bool mock_pred_i_p_p(const size_t i,       const void* const p1, const void* const p2);


void mock_free(void *ptr);


void *mock_clone(const void* const ptr);


char* mock_str(const void* const ptr);


void *mock_alloc(const void* const ptr);

#endif // MOCK_FN
