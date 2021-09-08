#include "calc.h"
#include "layout.h"
#include "laptop.h"
#include "listeners.h"

void desire_ltr(struct OutputManager *output_manager) {
	struct Head *head;
	struct SList *i;

	if (!output_manager)
		return;

	// head specific
	for (i = output_manager->heads; i; i = i->nex) {
		head = (struct Head*)i->val;

		head->desired.enabled = !closed_laptop_display(head->name);
		head->desired.mode = optimal_mode(head->modes);
		head->desired.scale = auto_scale(head);
	}

	// head order, including disabled
	slist_free(&output_manager->desired.heads);
	output_manager->desired.heads = order_heads(NULL, output_manager->heads);

	// head position
	ltr_heads(output_manager->desired.heads);
}

void apply_desired(struct OutputManager *output_manager) {
	struct Head *head;
	struct SList *i;
	struct zwlr_output_configuration_v1 *zwlr_config;
	struct zwlr_output_configuration_head_v1 *config_head;

	if (!output_manager)
		return;

	// passed into our configuration listener
	zwlr_config = zwlr_output_manager_v1_create_configuration(output_manager->zwlr_output_manager, output_manager->serial);
	zwlr_output_configuration_v1_add_listener(zwlr_config, output_configuration_listener(), output_manager);

	for (i = output_manager->desired.heads; i; i = i->nex) {
		head = (struct Head*)i->val;

		if (head->desired.enabled) {

			// just a handle for subsequet calls
			config_head = zwlr_output_configuration_v1_enable_head(zwlr_config, head->zwlr_head);
			head->pending.enabled = true;

			if (head->desired.mode) {
				zwlr_output_configuration_head_v1_set_mode(config_head, head->desired.mode->zwlr_mode);
				head->pending.mode = true;
			}

			zwlr_output_configuration_head_v1_set_scale(config_head, head->desired.scale);
			head->pending.scale = true;

			zwlr_output_configuration_head_v1_set_position(config_head, head->desired.x, head->desired.y);
			head->pending.position = true;

		} else {
			zwlr_output_configuration_v1_disable_head(zwlr_config, head->zwlr_head);
		}
	}

	zwlr_output_configuration_v1_apply(zwlr_config);
}

void print_desired(struct OutputManager *output_manager) {
	struct Head *head;
	struct SList *i;

	if (!output_manager)
		return;

	printf("\nProposed:\n");
	for (i = output_manager->heads; i; i = i->nex) {
		head = i->val;
		if (!head)
			continue;

		printf(" %s %s %dmm x %dmm\n",
				head->name,
				head->description,
				head->width_mm,
				head->height_mm
			  );

		printf("  from: ");
		if (head->enabled && head->current_mode) {
			printf("%dx%d@%ldHz %s  scale %.2f  position %d,%d\n",
					head->current_mode->width,
					head->current_mode->height,
					(long)(((double)head->current_mode->refresh_mHz / 1000 + 0.5)),
					head->current_mode->preferred ? "(preferred)" : "           ",
					wl_fixed_to_double(head->scale),
					head->x,
					head->y
				  );
		} else if (!head->enabled) {
			printf("disabled\n");
		} else {
			printf("no mode  scale %.2f  position %d,%d\n",
					wl_fixed_to_double(head->scale),
					head->x,
					head->y
				  );
		}

		printf("  to:   ");
		if (head->desired.enabled && head->desired.mode) {
			printf("%dx%d@%ldHz %s  scale %.2f  position %d,%d\n",
					head->desired.mode->width,
					head->desired.mode->height,
					(long)(((double)head->desired.mode->refresh_mHz / 1000 + 0.5)),
					head->desired.mode->preferred ? "(preferred)" : "           ",
					wl_fixed_to_double(head->desired.scale),
					head->desired.x,
					head->desired.y
				  );
		} else if (!head->desired.enabled) {
			printf("disabled\n");
		} else {
			printf("no mode scale %.2f  position %d,%d\n",
					wl_fixed_to_double(head->desired.scale),
					head->desired.x,
					head->desired.y
				  );
		}
	}

	fflush(stdout);
}

