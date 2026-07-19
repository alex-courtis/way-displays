#ifndef PSLIST_H
#define PSLIST_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * Containerless singly linked list.
 * NULL values permitted.
 */
struct Pslist {
	void *val;
	struct Pslist *nex;
};

/*
 * Lifecycle
 */

// clone a list, fn_clone for deep clone, NULL clone_val for shallow clone setting pointers only
struct Pslist *pslist_clone(struct Pslist *head, fn_clone clone_val);

// free list
void pslist_free(struct Pslist **head);

// free list and vals, NULL free_val uses free()
void pslist_free_vals(struct Pslist **head, fn_free free_val);

/*
 * Mutate
 */

// append val to a list
struct Pslist *pslist_append(struct Pslist **head, void *val);

// remove an item, returning the val
void *pslist_remove(struct Pslist **head, struct Pslist **item);

// remove items, NULL pred is val pointer comparison
size_t pslist_remove_from(struct Pslist **head, fn_pred_p_p pred, const void *data);

// remove items and free vals, NULL pred is val pointer comparison, NULL free_val calls free()
size_t pslist_remove_all_free(struct Pslist **head, fn_pred_p_p pred, const void *data, fn_free free_val);

// merges list2 into list1, such that the resulting list contains only elements that appeared exclusively in list1 or list2.
void pslist_xor_free(struct Pslist **head1, struct Pslist *head2, fn_pred_p_p pred, fn_free free_val, fn_clone clone_val);

/*
 * Access
 */

// val at position
void *pslist_at(const struct Pslist *head, size_t index);

// find
struct Pslist *pslist_find(struct Pslist *head, fn_pred_p pred);

// find a val
void *pslist_find_val(struct Pslist *head, fn_pred_p pred);

// find, NULL equal_val is val pointer comparison
struct Pslist *pslist_find_equal(struct Pslist *head, fn_equal equal_val, const void *b);

// find a val, NULL equal_val is val pointer comparison
void *pslist_find_equal_val(struct Pslist *head, fn_equal equal_val, const void *b);

/*
 * Comparison
 */

// same length and every item equal in order, NULL equal_val compares pointers
bool pslist_equal(struct Pslist *a, struct Pslist *b, fn_equal equal_val);

/*
 * Utility
 */

// insertion sort into a new list
struct Pslist *pslist_sort(struct Pslist *head, fn_less_than less_than_val);

// move items between lists where from value matches data, NULL pred does nothing
void pslist_move(struct Pslist **to, struct Pslist **from, fn_pred_p_p pred, const void *data);

/*
 * Info
 */

// to string, user frees, format "%p\n", "%s" when str_val
char *pslist_str(const struct Pslist *head, fn_str str_val);

// length
size_t pslist_length(const struct Pslist *head);

#endif // PSLIST_H

