#ifndef LIST_H
#define LIST_H

struct SList {
	void *val;
	struct SList *nex;
};

void slist_append(struct SList **head, void *val);

void slist_remove(struct SList **head, void *val);

void slist_free(struct SList **head);

#endif // LIST_H

