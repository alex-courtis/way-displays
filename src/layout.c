#include <stdio.h>
#include <string.h>
#include "calc.h"
#include "layout.h"
#include "lid.h"
#include "listeners.h"

wl_fixed_t scale_head(struct Head *head, struct Cfg *cfg) {
	struct UserScale *user_scale;

	for (struct SList *i = cfg->user_scales; i; i = i->nex) {
		user_scale = (struct UserScale*)i->val;
		if (user_scale->name_desc &&
				(strcmp(user_scale->name_desc, head->name) == 0 ||
				 strcmp(user_scale->name_desc, head->description) == 0)) {
			return wl_fixed_from_double(user_scale->scale);
		}
	}

	if (cfg->auto_scale) {
		return auto_scale(head);
	} else {
		return wl_fixed_from_int(1);
	}
}

void desire_ltr(struct Displ *displ) {
	struct Head *head;
	struct SList *i;

	if (!displ || !displ->output_manager || !displ->cfg)
		return;

	// head specific
	for (i = displ->output_manager->heads; i; i = i->nex) {
		head = (struct Head*)i->val;

		if ((head->desired.enabled = !head->lid_closed)) {
			head->desired.mode = optimal_mode(head->modes);
			head->desired.scale = scale_head(head, displ->cfg);
		}
	}

	// head order, including disabled
	slist_free(&displ->output_manager->desired.heads);
	displ->output_manager->desired.heads = order_heads(displ->cfg->order_name_desc, displ->output_manager->heads);

	// head position
	ltr_heads(displ->output_manager->desired.heads);
}

void pend_desired(struct Displ *displ) {
	struct Head *head;
	struct SList *i;

	if (!displ || !displ->output_manager)
		return;

	for (i = displ->output_manager->heads; i; i = i->nex) {
		head = (struct Head*)i->val;

		head->pending.enabled = head->desired.enabled != head->enabled;
		if (head->pending.enabled && head->desired.enabled) {
			// always confirm when enabling, as some settings (position) seem to be forgotten
			head->pending.mode = true;
			head->pending.scale = true;
			head->pending.position = true;
		} else {
			head->pending.mode = head->desired.mode && head->desired.mode != head->current_mode;
			head->pending.scale = head->desired.scale != head->scale;
			head->pending.position = (head->desired.x != head->x) || (head->desired.y != head->y);
		}
	}
}

void apply_desired(struct Displ *displ) {
	struct Head *head;
	struct SList *i;
	struct zwlr_output_configuration_v1 *zwlr_config;

	if (!displ || !displ->output_manager)
		return;

	// passed into our configuration listener
	zwlr_config = zwlr_output_manager_v1_create_configuration(displ->output_manager->zwlr_output_manager, displ->output_manager->serial);
	zwlr_output_configuration_v1_add_listener(zwlr_config, output_configuration_listener(), displ->output_manager);

	for (i = displ->output_manager->desired.heads; i; i = i->nex) {
		head = (struct Head*)i->val;

		if (head->desired.enabled) {

			// Just a handle for subsequent calls; it's why we always enable instead of just on changes.
			head->zwlr_config_head = zwlr_output_configuration_v1_enable_head(zwlr_config, head->zwlr_head);

			if (head->desired.mode && head->pending.mode) {
				zwlr_output_configuration_head_v1_set_mode(head->zwlr_config_head, head->desired.mode->zwlr_mode);
			}

			if (head->pending.scale) {
				zwlr_output_configuration_head_v1_set_scale(head->zwlr_config_head, head->desired.scale);
			}

			if (head->pending.position) {
				zwlr_output_configuration_head_v1_set_position(head->zwlr_config_head, head->desired.x, head->desired.y);
			}

		} else {
			zwlr_output_configuration_v1_disable_head(zwlr_config, head->zwlr_head);
		}
	}

	zwlr_output_configuration_v1_apply(zwlr_config);
}

