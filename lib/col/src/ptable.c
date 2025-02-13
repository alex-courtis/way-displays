#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "fn.h"
#include "itable.h"

#include "ptable.h"

const struct PTable *ptable_init(const size_t initial, const size_t grow) {

	// pointer has to fit into ITable key
	assert(sizeof(void*) <= sizeof(uint64_t));

	return (struct PTable *)itable_init(initial, grow);
}

void ptable_free(const void* const tab) {
	itable_free(tab);
}

void ptable_free_vals(const struct PTable* const tab, fn_free_val free_val) {
	itable_free_vals((const struct ITable* const)tab, free_val);
}

void ptable_iter_free(const struct PTableIter* const iter) {
	itable_iter_free((const struct ITableIter* const)iter);
}

const void *ptable_get(const struct PTable* const tab, const void* const key) {
	return itable_get((const struct ITable* const)tab, (const uint64_t)key);
}

const struct PTableIter *ptable_iter(const struct PTable* const tab) {
	return (struct PTableIter*)itable_iter((struct ITable* const)tab);
}

const struct PTableIter *ptable_next(const struct PTableIter* const iter) {
	return (struct PTableIter*)itable_next((struct ITableIter* const)iter);
}

const void *ptable_put(const struct PTable* const tab, const void* const key, const void* const val) {
	return itable_put((const struct ITable* const)tab, (const uint64_t)key, val);
}

const void *ptable_remove(const struct PTable* const tab, const void* const key) {
	return itable_remove((const struct ITable* const)tab, (const uint64_t)key);
}

bool ptable_equal(const struct PTable* const a, const struct PTable* const b, fn_equals equals) {
	return itable_equal((const struct ITable* const)a, (const struct ITable* const)b, equals);
}

struct SList *ptable_keys_slist(const struct PTable* const tab) {
	return itable_keys_slist((const struct ITable* const)tab);
}

struct SList *ptable_vals_slist(const struct PTable* const tab) {
	return itable_vals_slist((const struct ITable* const)tab);
}

size_t ptable_size(const struct PTable* const tab) {
	return itable_size((struct ITable* const)tab);
}

size_t ptable_capacity(const struct PTable* const tab) {
	return itable_capacity((struct ITable* const)tab);
}

