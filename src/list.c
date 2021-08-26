#include <stdlib.h>

#include "list.h"





#include <stdio.h>

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


	fprintf(stderr, "slist_remove head %p %p val %p\n", (void*)*head, (void*)head, (void*)val);
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
	fprintf(stderr, "slist_remove found %p\n", (void*)f);

	if (f) {
		if (p) {
			p->nex = f->nex;
		} else {
			*head = f->nex;
		}
		fprintf(stderr, "slist_remove reattached\n");
		free(f);
	}
	fprintf(stderr, "slist_remove done\n");
}

void slist_free(struct SList **head) {
	struct SList *i, *f;

	i = *head;
	while (i) {
		f = i;
		i = i->nex;
		/* fprintf(stderr, "freeing %s\n", (char*)f->val); */
		free(f);
	}

	*head = NULL;
}

