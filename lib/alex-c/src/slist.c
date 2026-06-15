#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "str.h"

#include "slist.h"

struct SList *slist_clone(struct SList *head, fn_clone clone_val) {
	struct SList *c, *i;

	c = NULL;
	for (i = head; i; i = i->nex) {
		if (clone_val) {
			slist_append(&c, clone_val(i->val));
		} else {
			slist_append(&c, i->val);
		}
	}

	return c;
}

void slist_free(struct SList **head) {
	struct SList *i = *head;
	while (i) {
		struct SList *f = i;
		i = i->nex;
		free(f);
	}

	*head = NULL;
}

void slist_free_vals(struct SList **head, fn_free free_val) {
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

void *slist_remove(struct SList **head, struct SList **item) {
	if (!item)
		return NULL;

	struct SList *i, *f, *p;
	void *removed = NULL;

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

size_t slist_remove_all(struct SList **head, fn_equal equal_val, const void *b) {
	struct SList *i;
	size_t removed = 0;

	while ((i = slist_find_equal(*head, equal_val, b))) {
		slist_remove(head, &i);
		removed++;
	}

	return removed;
}

size_t slist_remove_all_free(struct SList **head, fn_equal equal_val, const void *b, fn_free free_val) {
	struct SList *i;
	size_t removed = 0;

	while ((i = slist_find_equal(*head, equal_val, b))) {
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

void slist_xor_free(struct SList **head1, struct SList *head2, fn_equal equal_val, fn_free free_val, fn_clone clone_val) {
	struct SList *i = head2;

	while (i) {
		if (!slist_remove_all_free(head1, equal_val, i->val, free_val)) {
			if (clone_val) {
				slist_append(head1, clone_val(i->val));
			} else {
				slist_append(head1, i->val);
			}
		}

		i = i->nex;
	}
}

void *slist_at(const struct SList *head, size_t index) {
	size_t c = 0;
	for (const struct SList *i = head; i; i = i->nex, c++) {
		if (c == index) {
			return i->val;
		}
	}

	return NULL;
}

struct SList *slist_find(struct SList *head, fn_test test_val) {
	struct SList *i;

	if (!test_val)
		return NULL;

	for (i = head; i; i = i->nex) {
		if (test_val(i->val)) {
			return i;
		}
	}

	return NULL;
}

void *slist_find_val(struct SList *head, fn_test test_val) {
	const struct SList *f = slist_find(head, test_val);
	if (f)
		return f->val;
	else
		return NULL;
}

struct SList *slist_find_equal(struct SList *head, fn_equal equal_val, const void *b) {
	struct SList *i;

	for (i = head; i; i = i->nex) {
		if (equal_val) {
			if (equal_val(i->val, b)) {
				return i;
			}
		} else if (i->val == b) {
			return i;
		}
	}

	return NULL;
}

void *slist_find_equal_val(struct SList *head, fn_equal equal_val, const void *b) {
	const struct SList *f = slist_find_equal(head, equal_val, b);
	if (f)
		return f->val;
	else
		return NULL;
}

bool slist_equal(struct SList *a, struct SList *b, fn_equal equal_val) {
	struct SList *ai, *bi;

	for (ai = a, bi = b; ai && bi; ai = ai->nex, bi = bi->nex) {
		if (equal_val) {
			if (!equal_val(ai->val, bi->val)) {
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

size_t slist_length(const struct SList *head) {
	size_t length = 0;

	for (const struct SList *i = head; i; i = i->nex) {
		length++;
	}

	return length;
}

struct SList *slist_sort(struct SList *head, fn_less_than less_than_val) {
	struct SList *sorted = NULL;

	if (!head || !less_than_val) {
		return sorted;
	}

	if (!head->nex) {
		slist_append(&sorted, head->val);
		return sorted;
	}

	struct SList *sorting = slist_clone(head, NULL);

	struct SList *sorting_head;

	while (sorting != NULL) {
		struct SList **sorted_trail = &sorted;

		sorting_head = sorting;

		sorting = sorting->nex;

		while (!(*sorted_trail == NULL || less_than_val(sorting_head->val, (*sorted_trail)->val))) {
			sorted_trail = &(*sorted_trail)->nex;
		}

		sorting_head->nex = *sorted_trail;
		*sorted_trail = sorting_head;
	}

	slist_free(&sorting);
	return sorted;
}

void slist_move(struct SList **to, struct SList **from, fn_equal equal_val, const void *b) {
	if (!to || !from || !equal_val)
		return;

	struct SList *f = *from;
	while (f) {
		struct SList *r = f;
		void *val = f->val;
		f = f->nex;
		if (equal_val(val, b)) {
			slist_append(to, val);
			slist_remove(from, &r);
		}
	}
}

char *slist_str(const struct SList *head, fn_str str_val) {
	if (!head)
		return NULL;

	char *out = strdup("");

	for (const struct SList *i = head; i; i = i->nex) {
		if (i->val) {
			if (str_val) {
				char *val_str = str_val(i->val);
				out = sprintf_append(out, "%s\n", val_str);
				free(val_str);
			} else {
				out = sprintf_append(out, "%p\n", i->val);
			}
		} else {
			out = sprintf_append(out, "(null)\n");
		}
	}

	return out;
}

