#include "layout.h"

void print_proposed(struct OutputManager *output_manager) {
	struct Head *head;
	struct SList *i;

	if (!output_manager)
		return;

	printf("\nProposed:\n");
	for (i = output_manager->heads; i; i = i->nex) {
		head = i->val;

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
					head->current_mode->preferred ? "(preferred)" : "           ",
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

	// TODO remove
	fflush(stdout);
}

