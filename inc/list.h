#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

struct SList {
	void *val;
	struct SList *nex;
};

// append val to a list
struct SList *slist_append(struct SList **head, void *val);

// remove an item, returning the val
void *slist_remove(struct SList **head, struct SList **item);

// remove items, null predicate is val pointer comparison
unsigned long slist_remove_all(struct SList **head, bool (*predicate)(const void *val, const void *data), const void *data);

// remove items and free vals, null predicate is val pointer comparison, null free_val calls free()
unsigned long slist_remove_all_free(struct SList **head, bool (*predicate)(const void *val, const void *data), const void *data, void (*free_val)(void *val));

// find
struct SList *slist_find(struct SList *head, bool (*test)(const void *val));

// find a val
void *slist_find_val(struct SList *head, bool (*test)(const void *val));

// find, null predicate is val pointer comparison
struct SList *slist_find_equal(struct SList *head, bool (*predicate)(const void *val, const void *data), const void *data);

// find a val, null predicate is val pointer comparison
void *slist_find_equal_val(struct SList *head, bool (*predicate)(const void *val, const void *data), const void *data);

// same length and every item passes test in order, null equal compares pointers
bool slist_equal(struct SList *a, struct SList *b, bool (*equal)(const void *a, const void *b));

// length
unsigned long slist_length(struct SList *head);

// val at position
void *slist_at(struct SList *head, unsigned long index);

// clone the list, setting val pointers
struct SList *slist_shallow_clone(struct SList *head);

// sort into a new list
struct SList *slist_sort(struct SList *head, bool (*before)(const void *a, const void *b));

// move items between lists with predicate, null predicate does nothing
void slist_move(struct SList **to, struct SList **from, bool (*predicate)(const void *val, const void *data), const void *data);

// free list
void slist_free(struct SList **head);

// free list and vals, null free_val uses free()
void slist_free_vals(struct SList **head, void (*free_val)(void *val));

// test val for equality using strcasecmp
bool slist_equal_strcasecmp(const void *val, const void *data);

#endif // LIST_H

