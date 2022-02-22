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

struct SList *slist_find(struct SList **head, bool (*test)(const void *val, const void *data), const void *data) {
	struct SList *i;

	for (i = *head; i; i = i->nex) {
		if (test) {
			if (test(i->val, data)) {
				return i;
			}
		} else if (i->val == data) {
			return i;
		}
	}

	return NULL;
}

bool slist_equal(struct SList *a, struct SList *b, bool (*test)(const void *val, const void *data)) {
	struct SList *ai, *bi;

	for (ai = a, bi = b; ai && bi; ai = ai->nex, bi = bi->nex) {
		if (test) {
			if (!test(ai->val, bi->val)) {
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

unsigned long slist_remove_all(struct SList **head, bool (*test)(const void *val, const void *data), const void *data) {
	struct SList *i;
	unsigned long removed = 0;

	while ((i = slist_find(head, test, data))) {
		slist_remove(head, &i);
		removed++;
	}

	return removed;
}

unsigned long slist_remove_all_free(struct SList **head, bool (*test)(const void *val, const void *data), const void *data, void (*free_val)(void *val)) {
	struct SList *i;
	unsigned long removed = 0;

	while ((i = slist_find(head, test, data))) {
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

bool slist_test_strcasecmp(const void *val, const void *data) {
	if (!val || !data) {
		return false;
	}
	return strcasecmp(val, data) == 0;
}

