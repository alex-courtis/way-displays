#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "str.h"

#include "pslist.h"

struct Pslist *pslist_clone(struct Pslist *head, fn_clone clone_val) {
	struct Pslist *c, *i;

	c = NULL;
	for (i = head; i; i = i->nex) {
		if (clone_val) {
			pslist_append(&c, clone_val(i->val));
		} else {
			pslist_append(&c, i->val);
		}
	}

	return c;
}

void pslist_free(struct Pslist **head) {
	struct Pslist *i = *head;
	while (i) {
		struct Pslist *f = i;
		i = i->nex;
		free(f);
	}

	*head = NULL;
}

void pslist_free_vals(struct Pslist **head, fn_free free_val) {
	struct Pslist *i;

	for (i = *head; i; i = i->nex) {
		if (free_val) {
			free_val(i->val);
		} else {
			free(i->val);
		}
	}

	pslist_free(head);
}

struct Pslist *pslist_append(struct Pslist **head, void *val) {
	struct Pslist *i, *l;

	i = calloc(1, sizeof(struct Pslist));
	i->val = val;

	if (*head) {
		for (l = *head; l->nex; l = l->nex);
		l->nex = i;
	} else {
		*head = i;
	}

	return i;
}

void *pslist_remove(struct Pslist **head, struct Pslist **item) {
	if (!item)
		return NULL;

	struct Pslist *i, *f, *p;
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

size_t pslist_remove_all(struct Pslist **head, fn_2pred pred_val, const void *data) {
	struct Pslist *i;
	size_t removed = 0;

	while ((i = pslist_find_equal(*head, pred_val, data))) {
		pslist_remove(head, &i);
		removed++;
	}

	return removed;
}

size_t pslist_remove_all_free(struct Pslist **head, fn_2pred pred_val, const void *data, fn_free free_val) {
	struct Pslist *i;
	size_t removed = 0;

	while ((i = pslist_find_equal(*head, pred_val, data))) {
		if (free_val) {
			free_val(i->val);
		} else {
			free(i->val);
		}
		pslist_remove(head, &i);
		removed++;
	}

	return removed;
}

void pslist_xor_free(struct Pslist **head1, struct Pslist *head2, fn_2pred pred_val, fn_free free_val, fn_clone clone_val) {
	struct Pslist *i = head2;

	while (i) {
		if (!pslist_remove_all_free(head1, pred_val, i->val, free_val)) {
			if (clone_val) {
				pslist_append(head1, clone_val(i->val));
			} else {
				pslist_append(head1, i->val);
			}
		}

		i = i->nex;
	}
}

void *pslist_at(const struct Pslist *head, size_t index) {
	size_t c = 0;
	for (const struct Pslist *i = head; i; i = i->nex, c++) {
		if (c == index) {
			return i->val;
		}
	}

	return NULL;
}

struct Pslist *pslist_find(struct Pslist *head, fn_pred pred_val) {
	struct Pslist *i;

	if (!pred_val)
		return NULL;

	for (i = head; i; i = i->nex) {
		if (pred_val(i->val)) {
			return i;
		}
	}

	return NULL;
}

void *pslist_find_val(struct Pslist *head, fn_pred pred_val) {
	const struct Pslist *f = pslist_find(head, pred_val);
	if (f)
		return f->val;
	else
		return NULL;
}

struct Pslist *pslist_find_equal(struct Pslist *head, fn_equal equal_val, const void *b) {
	struct Pslist *i;

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

void *pslist_find_equal_val(struct Pslist *head, fn_equal equal_val, const void *b) {
	const struct Pslist *f = pslist_find_equal(head, equal_val, b);
	if (f)
		return f->val;
	else
		return NULL;
}

bool pslist_equal(struct Pslist *a, struct Pslist *b, fn_equal equal_val) {
	struct Pslist *ai, *bi;

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

size_t pslist_length(const struct Pslist *head) {
	size_t length = 0;

	for (const struct Pslist *i = head; i; i = i->nex) {
		length++;
	}

	return length;
}

struct Pslist *pslist_sort(struct Pslist *head, fn_less_than less_than_val) {
	struct Pslist *sorted = NULL;

	if (!head || !less_than_val) {
		return sorted;
	}

	if (!head->nex) {
		pslist_append(&sorted, head->val);
		return sorted;
	}

	struct Pslist *sorting = pslist_clone(head, NULL);

	struct Pslist *sorting_head;

	while (sorting != NULL) {
		struct Pslist **sorted_trail = &sorted;

		sorting_head = sorting;

		sorting = sorting->nex;

		while (!(*sorted_trail == NULL || less_than_val(sorting_head->val, (*sorted_trail)->val))) {
			sorted_trail = &(*sorted_trail)->nex;
		}

		sorting_head->nex = *sorted_trail;
		*sorted_trail = sorting_head;
	}

	pslist_free(&sorting);
	return sorted;
}

void pslist_move(struct Pslist **to, struct Pslist **from, fn_2pred pred_val, const void *data) {
	if (!to || !from || !pred_val)
		return;

	struct Pslist *f = *from;
	while (f) {
		struct Pslist *r = f;
		void *val = f->val;
		f = f->nex;
		if (pred_val(val, data)) {
			pslist_append(to, val);
			pslist_remove(from, &r);
		}
	}
}

char *pslist_str(const struct Pslist *head, fn_str str_val) {
	if (!head)
		return NULL;

	char *out = strdup("");

	for (const struct Pslist *i = head; i; i = i->nex) {
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

