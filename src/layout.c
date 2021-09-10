#include <string.h>
#include "calc.h"
#include "layout.h"
#include "laptop.h"
#include "listeners.h"

void desire_ltr(struct Displ *displ) {
	struct Head *head;
	struct SList *i;

	if (!displ || !displ->output_manager || !displ->cfg)
		return;

	// head specific
	for (i = displ->output_manager->heads; i; i = i->nex) {
		head = (struct Head*)i->val;

		head->lid_closed = closed_laptop_display(head->name, displ->cfg);
		if ((head->desired.enabled = !head->lid_closed)) {
			head->desired.mode = optimal_mode(head->modes);
			head->desired.scale = auto_scale(head);
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
		head->pending.mode = head->desired.mode && head->desired.mode != head->current_mode;
		head->pending.scale = head->desired.scale != head->scale;
		head->pending.position = (head->desired.x != head->x) || (head->desired.y != head->y);
	}
}

void apply_desired(struct Displ *displ) {
	struct Head *head;
	struct SList *i;
	struct zwlr_output_configuration_v1 *zwlr_config;
	struct zwlr_output_configuration_head_v1 *config_head;

	if (!displ || !displ->output_manager)
		return;

	// passed into our configuration listener
	zwlr_config = zwlr_output_manager_v1_create_configuration(displ->output_manager->zwlr_output_manager, displ->output_manager->serial);
	zwlr_output_configuration_v1_add_listener(zwlr_config, output_configuration_listener(), displ->output_manager);

	for (i = displ->output_manager->desired.heads; i; i = i->nex) {
		head = (struct Head*)i->val;

		if (head->desired.enabled) {

			// Just a handle for subsequent calls; it's why we always enable instead of just on changes.
			config_head = zwlr_output_configuration_v1_enable_head(zwlr_config, head->zwlr_head);

			if (head->desired.mode && head->pending.mode) {
				zwlr_output_configuration_head_v1_set_mode(config_head, head->desired.mode->zwlr_mode);
			}

			if (head->pending.scale) {
				zwlr_output_configuration_head_v1_set_scale(config_head, head->desired.scale);
			}

			if (head->pending.position) {
				zwlr_output_configuration_head_v1_set_position(config_head, head->desired.x, head->desired.y);
			}

		} else {
			zwlr_output_configuration_v1_disable_head(zwlr_config, head->zwlr_head);
		}
	}

	zwlr_output_configuration_v1_apply(zwlr_config);
}

void print_desired(struct Displ *displ) {
	struct Head *head;
	struct SList *i;

	if (!displ || !displ->output_manager)
		return;

	printf("\nChanging:\n");
	for (i = displ->output_manager->heads; i; i = i->nex) {
		head = i->val;
		if (!head)
			continue;

		printf("  %s %s %dmm x %dmm\n",
				head->name,
				head->description,
				head->width_mm,
				head->height_mm
			  );

		printf("    from:\n");
		printf("      scale:    %.2f\n      position: %d,%d\n",
				wl_fixed_to_double(head->scale),
				head->x,
				head->y
			  );
		if (head->current_mode) {
			printf("      mode:     %dx%d@%ldHz %s\n",
					head->current_mode->width,
					head->current_mode->height,
					(long)(((double)head->current_mode->refresh_mHz / 1000 + 0.5)),
					head->current_mode->preferred ? "(preferred)" : "           "
				  );
		} else {
			printf("      (no mode)\n");
		}
		if (!head->enabled) {
			printf("      (disabled)\n");
		}
		if (head->lid_closed) {
			printf("      (lid closed)\n");
		}

		printf("    to:\n");
		if (is_pending_head(head)) {
			if (head->desired.enabled) {
				if (head->pending.scale) {
					printf("      scale:    %.2f\n",
							wl_fixed_to_double(head->desired.scale)
						  );
				}
				if (head->pending.position) {
					printf("      position: %d,%d\n",
							head->desired.x,
							head->desired.y
						  );
				}
				if (head->pending.mode && head->desired.mode) {
					printf("      mode:     %dx%d@%ldHz %s\n",
							head->desired.mode->width,
							head->desired.mode->height,
							(long)(((double)head->desired.mode->refresh_mHz / 1000 + 0.5)),
							head->desired.mode->preferred ? "(preferred)" : "           "
						  );
				}
				if (head->pending.enabled && head->desired.enabled) {
					printf("      (enabled)\n");
				}
			} else {
				printf("      (disabled)\n");
			}
		} else {
			printf("      (no change)\n");
		}
	}

	fflush(stdout);
}

