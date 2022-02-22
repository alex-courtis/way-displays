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

// remove items, null test is val pointer comparison
unsigned long slist_remove_all(struct SList **head, bool (*test)(const void *val, const void *data), const void *data);

// remove items and free vals, null test is val pointer comparison, null free_val calls free()
unsigned long slist_remove_all_free(struct SList **head, bool (*test)(const void *val, const void *data), const void *data, void (*free_val)(void *val));

// find a val, null test is val pointer comparison
struct SList *slist_find(struct SList **head, bool (*test)(const void *val, const void *data), const void *data);

// same length and every item passes test in order
bool slist_equal(struct SList *a, struct SList *b, bool (*test)(const void *val, const void *data));

// length
unsigned long slist_length(struct SList *head);

// clone the list, setting val pointers
struct SList *slist_shallow_clone(struct SList *head);

// free list
void slist_free(struct SList **head);

// free list and vals, null free_val uses free()
void slist_free_vals(struct SList **head, void (*free_val)(void *val));

// test val for equality using strcasecmp
bool slist_test_strcasecmp(const void *val, const void *data);

#endif // LIST_H

