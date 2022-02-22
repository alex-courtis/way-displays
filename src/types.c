#include <stdbool.h>
#include <stdlib.h>

#include "types.h"

#include "cfg.h"
#include "lid.h"
#include "list.h"

void free_mode(void *data) {
	struct Mode *mode = data;

	if (!mode)
		return;

	free(mode);
}

void free_head(void *data) {
	struct Head *head = data;

	if (!head)
		return;

	slist_free_vals(&head->modes, free_mode);

	free(head->name);
	free(head->description);
	free(head->make);
	free(head->model);
	free(head->serial_number);

	free(head);
}

void free_output_manager(void *data) {
	struct OutputManager *output_manager = data;

	if (!output_manager)
		return;

	slist_free_vals(&output_manager->heads, free_head);
	slist_free_vals(&output_manager->heads_departed, free_head);

	slist_free(&output_manager->heads_arrived);
	slist_free(&output_manager->desired.heads);

	free(output_manager->interface);

	free(output_manager);
}

void free_displ(void *data) {
	struct Displ *displ = data;

	if (!displ)
		return;

	free_output_manager(displ->output_manager);

	free_cfg(displ->cfg);

	free_lid(displ->lid);

	free(displ);
}

void head_free_mode(struct Head *head, struct Mode *mode) {
	if (!head || !mode)
		return;

	head->dirty = true;

	if (head->desired.mode == mode) {
		head->desired.mode = NULL;
	}
	if (head->current_mode == mode) {
		head->current_mode = NULL;
	}

	slist_remove_all(&head->modes, NULL, mode);

	free_mode(mode);
}

bool is_dirty(struct Displ *displ) {
	struct SList *i;
	struct Head *head;

	if (!displ)
		return false;

	if (displ->cfg && displ->cfg->dirty)
		return true;

	if (displ->lid && displ->lid->dirty)
		return true;

	if (!displ->output_manager)
		return false;

	if (displ->output_manager->dirty) {
		return true;
	}

	for (i = displ->output_manager->heads; i; i = i->nex) {
		head = i->val;
		if (!head)
			continue;

		if (head->dirty) {
			return true;
		}
	}

	return false;
}

void reset_dirty(struct Displ *displ) {
	struct SList *i;
	struct Head *head;

	if (!displ)
		return;

	if (displ->cfg)
		displ->cfg->dirty = false;

	if (displ->lid)
		displ->lid->dirty = false;

	if (!displ->output_manager)
		return;

	displ->output_manager->dirty = false;

	for (i = displ->output_manager->heads; i; i = i->nex) {
		head = i->val;
		if (!head)
			continue;

		head->dirty = false;
	}
}

bool is_pending_output_manager(struct OutputManager *output_manager) {
	struct SList *i;
	struct Head *head;

	if (!output_manager)
		return false;

	for (i = output_manager->heads; i; i = i->nex) {
		head = i->val;

		if (is_pending_head(head)) {
			return true;
		}
	}

	return false;
}

bool is_pending_head(struct Head *head) {
	return (head &&
			(head->pending.mode ||
			 head->pending.scale ||
			 head->pending.enabled ||
			 head->pending.position));
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

