#include <stdlib.h>

#include "list.h"

void slist_append(struct SList **head, void *val) {
	struct SList *i, *l;

	i = calloc(1, sizeof(struct SList));
	i->val = val;

	if (*head) {
        for (l = *head; l->nex; l = l->nex);
        l->nex = i;
	} else {
		*head = i;
	}
}

void slist_remove(struct SList **head, struct SList **item) {
	struct SList *i, *f, *p;

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
		free(f);
		*item = NULL;
	}
}

void slist_remove_all(struct SList **head, void *val) {
	struct SList *i, *r;

	i = *head;
	while(i) {
		r = i;
		i = i->nex;
		if (r->val == val) {
			slist_remove(head, &r);
		}
	}
}

struct SList *slist_shallow_clone(struct SList *head) {
	struct SList *c, *i;

	c = NULL;
	for (i = head; i; i = i->nex) {
		slist_append(&c, i->val);
	}

	return c;
}

long slist_length(struct SList *head) {
	long length = 0;

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

