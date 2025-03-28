#ifndef SLIST_H
#define SLIST_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

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

// free list and vals, NULL fn_free_val uses free()
void slist_free_vals(struct SList **head, fn_free_val);

/*
 * Mutate
 */

// append val to a list
struct SList *slist_append(struct SList **head, void *val);

// remove an item, returning the val
void *slist_remove(struct SList **head, struct SList **item);

// remove items, NULL fn_equals is val pointer comparison
size_t slist_remove_all(struct SList **head, fn_equals, const void *b);

// remove items and free vals, NULL equals is val pointer comparison, NULL fn_free_val calls free()
size_t slist_remove_all_free(struct SList **head, fn_equals, const void *b, fn_free_val);

// merges list2 into list1, such that the resulting list contains only elements that appeared exclusively in list1 or list2.
size_t slist_xor_free(struct SList **head1, struct SList *head2, fn_equals, fn_free_val, fn_copy_val);

/*
 * Access
 */

// val at position
void *slist_at(struct SList *head, size_t index);

// find
struct SList *slist_find(struct SList *head, fn_test);

// find a val
void *slist_find_val(struct SList *head, fn_test);

// find, NULL fn_equals is val pointer comparison
struct SList *slist_find_equal(struct SList *head, fn_equals, const void *b);

// find a val, NULL fn_equals is val pointer comparison
void *slist_find_equal_val(struct SList *head, fn_equals, const void *b);

/*
 * Comparison
 */

// same length and every item equal in order, NULL fn_equals compares pointers
bool slist_equal(struct SList *a, struct SList *b, fn_equals);

/*
 * Utility
 */

// sort into a new list
struct SList *slist_sort(struct SList *head, fn_less_than);

// move items between lists where from value equals b, NULL fn_equals does nothing
void slist_move(struct SList **to, struct SList **from, fn_equals, const void *b);

// move items between lists where from value equals b, NULL fn_equals does nothing
void slist_move(struct SList **to, struct SList **from, fn_equals, const void *b);

/*
 * Info
 */

// to string, user frees
// values must be char*, printed using %s
char *slist_str(struct SList *head);

// length
size_t slist_length(struct SList *head);

#endif // SLIST_H

