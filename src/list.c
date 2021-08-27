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

void slist_remove(struct SList **head, void *val) {
	struct SList *i, *f, *p;

	i = *head;
	p = NULL;
	f = NULL;

	for (i = *head; i; i = i->nex) {
		if (val == i->val) {
			f = i;
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

