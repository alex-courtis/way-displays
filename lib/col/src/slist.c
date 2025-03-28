#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"

#include "slist.h"

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

void slist_free_vals(struct SList **head, fn_free_val free_val) {
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

size_t slist_remove_all(struct SList **head, fn_equals equals, const void *b) {
	struct SList *i;
	size_t removed = 0;

	while ((i = slist_find_equal(*head, equals, b))) {
		slist_remove(head, &i);
		removed++;
	}

	return removed;
}

size_t slist_remove_all_free(struct SList **head, fn_equals equals, const void *b, fn_free_val free_val) {
	struct SList *i;
	size_t removed = 0;

	while ((i = slist_find_equal(*head, equals, b))) {
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

size_t slist_xor_free(struct SList **head1, struct SList *head2, fn_equals equals, fn_free_val free_val, fn_copy_val copy_val) {
	struct SList *i = head2;
	size_t removed = 0;

	while (i) {
		size_t removed = slist_remove_all_free(head1, equals, i->val, free_val);

		if (!removed) {
			if (copy_val) {
				slist_append(head1, copy_val(i->val));
			} else {
				slist_append(head1, i->val);
			}
		}

		i = i->nex;
	}

	return removed;
}

void *slist_at(struct SList *head, size_t index) {
	size_t c = 0;
	for (struct SList *i = head; i; i = i->nex, c++) {
		if (c == index) {
			return i->val;
		}
	}

	return NULL;
}

struct SList *slist_find(struct SList *head, fn_test test) {
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

void *slist_find_val(struct SList *head, fn_test test) {
	struct SList *f = slist_find(head, test);
	if (f)
		return f->val;
	else
		return NULL;
}

struct SList *slist_find_equal(struct SList *head, fn_equals equals, const void *b) {
	struct SList *i;

	for (i = head; i; i = i->nex) {
		if (equals) {
			if (equals(i->val, b)) {
				return i;
			}
		} else if (i->val == b) {
			return i;
		}
	}

	return NULL;
}

void *slist_find_equal_val(struct SList *head, fn_equals equals, const void *b) {
	struct SList *f = slist_find_equal(head, equals, b);
	if (f)
		return f->val;
	else
		return NULL;
}

bool slist_equal(struct SList *a, struct SList *b, fn_equals equals) {
	struct SList *ai, *bi;

	for (ai = a, bi = b; ai && bi; ai = ai->nex, bi = bi->nex) {
		if (equals) {
			if (!equals(ai->val, bi->val)) {
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

size_t slist_length(struct SList *head) {
	size_t length = 0;

	for (struct SList *i = head; i; i = i->nex) {
		length++;
	}

	return length;
}

struct SList *slist_sort(struct SList *head, fn_less_than less_than) {
	struct SList *sorted = NULL;

	if (!head || !less_than) {
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

		while (!(*sorted_trail == NULL || less_than(sorting_head->val, (*sorted_trail)->val))) {
			sorted_trail = &(*sorted_trail)->nex;
		}

		sorting_head->nex = *sorted_trail;
		*sorted_trail = sorting_head;
	}

	slist_free(&sorting);
	return sorted;
}

void slist_move(struct SList **to, struct SList **from, fn_equals equals, const void *b) {
	if (!to || !from || !equals)
		return;

	struct SList *f = *from;
	while (f) {
		struct SList *r = f;
		void *val = f->val;
		f = f->nex;
		if (equals(val, b)) {
			slist_append(to, val);
			slist_remove(from, &r);
		}
	}
}

char *slist_str(struct SList *head) {
	if (!head)
		return NULL;

	size_t len = 1;

	// calculate length
	// slower but simpler than realloc, which can set off scanners/checkers
	for (struct SList *i = head; i; i = i->nex) {
		len += strlen(i->val) + 1;
	}

	// render
	char *buf = (char*)calloc(len, sizeof(char));
	char *bufp = buf;
	for (struct SList *i = head; i; i = i->nex) {
		bufp += snprintf(bufp, len - (bufp - buf), "%s\n", (char*)i->val);
	}

	// strip trailing newline
	if (bufp > buf) {
		*(bufp - 1) = '\0';
	}

	return buf;
}

