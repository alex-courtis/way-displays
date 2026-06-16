#ifndef SLIST_H
#define SLIST_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * Containerless singly linked list.
 * NULL values permitted.
 */
struct SList {
	void *val;
	struct SList *nex;
};

/*
 * Lifecycle
 */

// clone a list, fn_clone for deep clone, NULL clone_val for shallow clone setting pointers only
struct SList *slist_clone(struct SList *head, fn_clone clone_val);

// free list
void slist_free(struct SList **head);

// free list and vals, NULL free_val uses free()
void slist_free_vals(struct SList **head, fn_free free_val);

/*
 * Mutate
 */

// append val to a list
struct SList *slist_append(struct SList **head, void *val);

// remove an item, returning the val
void *slist_remove(struct SList **head, struct SList **item);

// remove items, NULL equal_val is val pointer comparison
size_t slist_remove_all(struct SList **head, fn_equal equal_val, const void *b);

// remove items and free vals, NULL equal_val is val pointer comparison, NULL free_val calls free()
size_t slist_remove_all_free(struct SList **head, fn_equal equal_val, const void *b, fn_free free_val);

// merges list2 into list1, such that the resulting list contains only elements that appeared exclusively in list1 or list2.
void slist_xor_free(struct SList **head1, struct SList *head2, fn_equal equal_val, fn_free free_val, fn_clone clone_val);

/*
 * Access
 */

// val at position
void *slist_at(const struct SList *head, size_t index);

// find
struct SList *slist_find(struct SList *head, fn_test test_val);

// find a val
void *slist_find_val(struct SList *head, fn_test test_val);

// find, NULL equal_val is val pointer comparison
struct SList *slist_find_equal(struct SList *head, fn_equal equal_val, const void *b);

// find a val, NULL equal_val is val pointer comparison
void *slist_find_equal_val(struct SList *head, fn_equal equal_val, const void *b);

/*
 * Comparison
 */

// same length and every item equal in order, NULL equal_val compares pointers
bool slist_equal(struct SList *a, struct SList *b, fn_equal equal_val);

/*
 * Utility
 */

// insertion sort into a new list
struct SList *slist_sort(struct SList *head, fn_less_than less_than_val);

// move items between lists where from value equals b, NULL equal_val does nothing
void slist_move(struct SList **to, struct SList **from, fn_equal equal_val, const void *b);

/*
 * Info
 */

// to string, user frees, format "%p\n", "%s" when str_val
char *slist_str(const struct SList *head, fn_str str_val);

// length
size_t slist_length(const struct SList *head);

#endif // SLIST_H

