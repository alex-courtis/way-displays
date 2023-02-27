#include <stdbool.h>
#include <stdlib.h>
#include <strings.h>

#include "list.h"

struct SList *slist_append(struct SList **head, void *val) {
	struct SList *i, *l;

	i = calloc(1, sizeof(struct SList));
	i->val = val;

	if (*head) {
		for (l = *head; l->nex; l = l->nex);
		l->nex = i;
	} else {
		*head = i;
	}

	return i;
}

struct SList *slist_find(struct SList *head, bool (*test)(const void *val)) {
	struct SList *i;

	if (!test)
		return NULL;

	for (i = head; i; i = i->nex) {
		if (test(i->val)) {
			return i;
		}
	}

	return NULL;
}

void *slist_find_val(struct SList *head, bool (*test)(const void *val)) {
	struct SList *f = slist_find(head, test);
	if (f)
		return f->val;
	else
		return NULL;
}

struct SList *slist_find_equal(struct SList *head, bool (*predicate)(const void *val, const void *data), const void *data) {
	struct SList *i;

	for (i = head; i; i = i->nex) {
		if (predicate) {
			if (predicate(i->val, data)) {
				return i;
			}
		} else if (i->val == data) {
			return i;
		}
	}

	return NULL;
}

void *slist_find_equal_val(struct SList *head, bool (*predicate)(const void *val, const void *data), const void *data) {
	struct SList *f = slist_find_equal(head, predicate, data);
	if (f)
		return f->val;
	else
		return NULL;
}

bool slist_equal(struct SList *a, struct SList *b, bool (*equal)(const void *a, const void *b)) {
	struct SList *ai, *bi;

	for (ai = a, bi = b; ai && bi; ai = ai->nex, bi = bi->nex) {
		if (equal) {
			if (!equal(ai->val, bi->val)) {
				return false;
			}
		} else if (ai->val != bi->val) {
			return false;
		}
	}

	if (ai || bi) {
		return false;
	}

	return true;
}

void *slist_remove(struct SList **head, struct SList **item) {
	struct SList *i, *f, *p;
	void *removed = NULL;

	i = *head;
	p = NULL;
	f = NULL;

	for (i = *head; i; i = i->nex) {
		if (i == *item) {
			f = *item;
			break;
		}
		p = i;
	}

	if (f) {
		if (p) {
			p->nex = f->nex;
		} else {
			*head = f->nex;
		}
		removed = f->val;
		free(f);
		*item = NULL;
	}

	return removed;
}

unsigned long slist_remove_all(struct SList **head, bool (*predicate)(const void *val, const void *data), const void *data) {
	struct SList *i;
	unsigned long removed = 0;

	while ((i = slist_find_equal(*head, predicate, data))) {
		slist_remove(head, &i);
		removed++;
	}

	return removed;
}

unsigned long slist_remove_all_free(struct SList **head, bool (*predicate)(const void *val, const void *data), const void *data, void (*free_val)(void *val)) {
	struct SList *i;
	unsigned long removed = 0;

	while ((i = slist_find_equal(*head, predicate, data))) {
		if (free_val) {
			free_val(i->val);
		} else {
			free(i->val);
		}
		slist_remove(head, &i);
		removed++;
	}

	return removed;
}

struct SList *slist_shallow_clone(struct SList *head) {
	struct SList *c, *i;

	c = NULL;
	for (i = head; i; i = i->nex) {
		slist_append(&c, i->val);
	}

	return c;
}

unsigned long slist_length(struct SList *head) {
	unsigned long length = 0;

	for (struct SList *i = head; i; i = i->nex) {
		length++;
	}

	return length;
}

void *slist_at(struct SList *head, unsigned long index) {
	unsigned long c = 0;
	for (struct SList *i = head; i; i = i->nex, c++) {
		if (c == index) {
			return i->val;
		}
	}

	return NULL;
}

struct SList *slist_sort(struct SList *head, bool (*before)(const void *a, const void *b)) {
	struct SList *sorted = NULL;

	if (!head || !before) {
		return sorted;
	}

	if (!head->nex) {
		slist_append(&sorted, head->val);
		return sorted;
	}

	struct SList *sorting = slist_shallow_clone(head);

	struct SList *sorting_head  = sorting;
	struct SList **sorted_trail = &sorted;

	while (sorting != NULL) {
		sorting_head = sorting;
		sorted_trail = &sorted;

		sorting = sorting->nex;

		while (!(*sorted_trail == NULL || before(sorting_head->val, (*sorted_trail)->val))) {
			sorted_trail = &(*sorted_trail)->nex;
		}

		sorting_head->nex = *sorted_trail;
		*sorted_trail = sorting_head;
	}

	slist_free(&sorting);
	return sorted;
}

void slist_move(struct SList **to, struct SList **from, bool (*predicate)(const void *val, const void *data), const void *data) {
	if (!to || !from || !predicate)
		return;

	struct SList *f = *from;
	while (f) {
		struct SList *r = f;
		void *val = f->val;
		f = f->nex;
		if (predicate(val, data)) {
			slist_append(to, val);
			slist_remove(from, &r);
		}
	}
}

void slist_free(struct SList **head) {
	struct SList *i, *f;

	i = *head;
	while (i) {
		f = i;
		i = i->nex;
		free(f);
	}

	*head = NULL;
}

void slist_free_vals(struct SList **head, void (*free_val)(void *val)) {
	struct SList *i;

	for (i = *head; i; i = i->nex) {
		if (free_val) {
			free_val(i->val);
		} else {
			free(i->val);
		}
	}

	slist_free(head);
}

bool slist_equal_strcasecmp(const void *val, const void *data) {
	if (!val || !data) {
		return false;
	}
	return strcasecmp(val, data) == 0;
}

