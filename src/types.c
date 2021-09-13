#include "types.h"

void free_mode(struct Mode *mode) {
	if (!mode)
		return;

	free(mode);
}

void free_head(struct Head *head) {
	if (!head)
		return;

	for (struct SList *i = head->modes; i; i = i->nex) {
		free_mode(i->val);
	}
	slist_free(&head->modes);

	free(head->name);
	free(head->description);
	free(head->make);
	free(head->model);
	free(head->serial_number);

	free(head);
}

void free_output_manager(struct OutputManager *output_manager) {
	if (!output_manager)
		return;

	for (struct SList *i = output_manager->heads; i; i = i->nex) {
		free_head(i->val);
	}
	slist_free(&output_manager->heads);
	slist_free(&output_manager->desired.heads);

	output_manager_free_heads_departed(output_manager);

	free(output_manager);
}

void free_displ(struct Displ *displ) {
	if (!displ)
		return;

	free_output_manager(displ->output_manager);

	free_cfg(displ->cfg);

	free(displ);
}

void free_cfg(struct Cfg *cfg) {
	if (!cfg)
		return;

	for (struct SList *i = cfg->order_name_desc; i; i = i->nex) {
		free(i->val);
	}
	slist_free(&cfg->order_name_desc);

	free(cfg);
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

	slist_remove(&head->modes, mode);
}

void output_manager_release_head(struct OutputManager *output_manager, struct Head *head) {
	if (!output_manager || !head)
		return;

	output_manager->dirty = true;

	slist_remove(&output_manager->desired.heads, head);
	slist_remove(&output_manager->heads, head);
}

void output_manager_free_heads_departed(struct OutputManager *output_manager) {
	struct SList *i;
	struct Head *head;

	if (!output_manager)
		return;

	i = output_manager->heads_departed;
	while(i) {
		head = i->val;
		i = i->nex;

		slist_remove(&output_manager->heads_departed, head);
		free(head);
	}
}

bool is_dirty(struct OutputManager *output_manager) {
	struct SList *i;
	struct Head *head;

	if (!output_manager)
		return false;

	if (output_manager->dirty) {
		return true;
	}

	for (i = output_manager->heads; i; i = i->nex) {
		head = i->val;
		if (!head)
			continue;

		if (head->dirty) {
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

