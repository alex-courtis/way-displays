#ifndef LIST_H
#define LIST_H

struct SList {
	void *val;
	struct SList *nex;
};

void slist_append(struct SList **head, void *val);

void slist_remove(struct SList **head, struct SList **item);

void slist_remove_all(struct SList **head, void *val);

struct SList *slist_shallow_clone(struct SList *head);

void slist_free(struct SList **head);

#endif // LIST_H

