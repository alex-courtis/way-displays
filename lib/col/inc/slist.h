#ifndef SLIST_H
#define SLIST_H

#include <stdbool.h>
#include <stddef.h>

struct SList {
	void *val;
	struct SList *nex;
};

/*
 * Lifecycle
 */

// clone the list, setting val pointers
struct SList *slist_shallow_clone(struct SList *head);

// free list
void slist_free(struct SList **head);

// free list and vals, NULL free_val uses free()
void slist_free_vals(struct SList **head, void (*free_val)(void *val));

/*
 * Mutate
 */

// append val to a list
struct SList *slist_append(struct SList **head, void *val);

// remove an item, returning the val
void *slist_remove(struct SList **head, struct SList **item);

// remove items, NULL predicate is val pointer comparison
size_t slist_remove_all(struct SList **head, bool (*predicate)(const void *val, const void *data), const void *data);

// remove items and free vals, NULL predicate is val pointer comparison, NULL free_val calls free()
size_t slist_remove_all_free(struct SList **head, bool (*predicate)(const void *val, const void *data), const void *data, void (*free_val)(void *val));

/*
 * Access
 */

// val at position
void *slist_at(struct SList *head, size_t index);

// find
struct SList *slist_find(struct SList *head, bool (*test)(const void *val));

// find a val
void *slist_find_val(struct SList *head, bool (*test)(const void *val));

// find, NULL predicate is val pointer comparison
struct SList *slist_find_equal(struct SList *head, bool (*predicate)(const void *val, const void *data), const void *data);

// find a val, NULL predicate is val pointer comparison
void *slist_find_equal_val(struct SList *head, bool (*predicate)(const void *val, const void *data), const void *data);

// same length and every item passes test in order, NULL equal compares pointers
bool slist_equal(struct SList *a, struct SList *b, bool (*equal)(const void *a, const void *b));

/*
 * Utility
 */

// length
size_t slist_length(struct SList *head);

// sort into a new list
struct SList *slist_sort(struct SList *head, bool (*before)(const void *a, const void *b));

// move items between lists with predicate, NULL predicate does nothing
void slist_move(struct SList **to, struct SList **from, bool (*predicate)(const void *val, const void *data), const void *data);

/*
 * Predicate
 */

// test val for equality using strcmp
bool slist_predicate_strcmp(const void *val, const void *data);

#endif // SLIST_H

