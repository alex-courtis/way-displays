#include "types.h"

void free_mode(struct Mode *mode) {
	/* fprintf(stderr, "free_mode %p\n", (void*)mode); */
	if (!mode)
		return;

	free(mode);
}

void free_head(struct Head *head) {
	/* fprintf(stderr, "free_head %p\n", (void*)head); */
	if (!head)
		return;

	for (struct SList *i = head->modes; i; i = i->nex) {
		free_mode(i->val);
	}
	slist_free(&head->modes);

	free(head);
}

void free_output_manager(struct OutputManager *output_manager) {
	/* fprintf(stderr, "free_output_manager %p\n", (void*)output_manager); */
	if (!output_manager)
		return;

	for (struct SList *i = output_manager->heads; i; i = i->nex) {
		free_head(i->val);
	}
	slist_free(&output_manager->heads);
	slist_free(&output_manager->desired.heads);

	free(output_manager);
}

void free_displ(struct Displ *displ) {
	fprintf(stderr, "free_displ %p\n", (void*)displ);
	if (!displ)
		return;

	free_output_manager(displ->output_manager);

	free(displ);
}

void head_release_mode(struct Head *head, struct Mode *mode) {
	if (!head || !mode)
		return;

	head->dirty = true;

	if (head->desired.mode == mode) {
		head->desired.mode = NULL;
	}
	if (head->current_mode == mode) {
		head->current_mode = NULL;
	}

	/* fprintf(stderr, "head_release_mode mode %dx%d\n", mode->width, mode->height); */

	slist_remove(&head->modes, mode);
}

void output_manager_release_head(struct OutputManager *output_manager, struct Head *head) {
	if (!output_manager || !head)
		return;

	output_manager->dirty = true;

	/* fprintf(stderr, "output_manager_release_head head %s\n", head->name); */

	slist_remove(&output_manager->desired.heads, head);
	slist_remove(&output_manager->heads, head);
}

bool is_dirty(struct OutputManager *output_manager) {
	struct SList *i;
	struct Head *head;

	if (!output_manager)
		return false;

	if (output_manager->dirty) {
		fprintf(stderr, "dirty output_manager\n");
		return true;
	}

	for (i = output_manager->heads; i; i = i->nex) {
		head = i->val;
		if (!head)
			continue;

		if (head->dirty) {
			fprintf(stderr, "dirty head %s\n", head->name);
			return true;
		}
	}

	return false;
}

void reset_dirty(struct OutputManager *output_manager) {
	struct SList *i;
	struct Head *head;

	if (!output_manager)
		return;

	output_manager->dirty = false;

	for (i = output_manager->heads; i; i = i->nex) {
		head = i->val;
		if (!head)
			continue;

		head->dirty = false;
	}
}

bool is_pending(struct OutputManager *output_manager) {
	struct SList *i;
	struct Head *head;

	if (!output_manager)
		return false;

	for (i = output_manager->heads; i; i = i->nex) {
		head = i->val;
		if (!head)
			continue;

		if (head->pending.mode ||
				head->pending.scale ||
				head->pending.enabled ||
				head->pending.position) {
			return true;
		}
	}

	return false;
}

void reset_pending_desired(struct OutputManager *output_manager) {
	struct SList *i;
	struct Head *head;

	if (!output_manager)
		return;

	slist_free(&output_manager->desired.heads);

	for (i = output_manager->heads; i; i = i->nex) {
		head = i->val;
		if (!head)
			continue;

		head->pending.mode = false;
		head->pending.scale = false;
		head->pending.enabled = false;
		head->pending.position = false;

		head->desired.mode = NULL;
		head->desired.scale = 0;
		head->desired.enabled = false;
		head->desired.x = 0;
		head->desired.y = 0;
	}
}

