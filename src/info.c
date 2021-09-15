#include "info.h"

void print_head_current(struct Head *head) {
	if (!head)
		return;

	printf("    scale:    %.2f\n    position: %d,%d\n",
			wl_fixed_to_double(head->scale),
			head->x,
			head->y
		  );
	if (head->current_mode) {
		printf("    mode:     %dx%d@%ldHz %s\n",
				head->current_mode->width,
				head->current_mode->height,
				(long)(((double)head->current_mode->refresh_mHz / 1000 + 0.5)),
				head->current_mode->preferred ? "(preferred)" : "           "
			  );
	} else {
		printf("    (no mode)\n");
	}
	if (!head->enabled) {
		printf("    (disabled)\n");
	}
	if (head->lid_closed) {
		printf("    (lid closed)\n");
	}
}

void print_head_desired(struct Head *head) {
	if (!head)
		return;

	if (is_pending_head(head)) {
		if (head->desired.enabled) {
			if (head->pending.scale) {
				printf("    scale:    %.2f",
						wl_fixed_to_double(head->desired.scale)
					  );
				if (!head->size_specified) {
					printf(" (default, size not specified)");
				}
				printf("\n");
			}
			if (head->pending.position) {
				printf("    position: %d,%d\n",
						head->desired.x,
						head->desired.y
					  );
			}
			if (head->pending.mode && head->desired.mode) {
				printf("    mode:     %dx%d@%ldHz %s\n",
						head->desired.mode->width,
						head->desired.mode->height,
						(long)(((double)head->desired.mode->refresh_mHz / 1000 + 0.5)),
						head->desired.mode->preferred ? "(preferred)" : "           "
					  );
			}
			if (head->pending.enabled && head->desired.enabled) {
				printf("    (enabled)\n");
			}
		} else {
			printf("    (disabled)\n");
		}
	} else {
		printf("    (no change)\n");
	}
}

void print_heads(enum event event, struct SList *heads) {
	struct Head *head;
	struct SList *i;

	if (event == DELTA)
		printf("\n");

	for (i = heads; i; i = i->nex) {
		head = i->val;
		if (!head)
			continue;

		switch (event) {
			case ARRIVED:
				printf("\n%s Arrived:\n", head->name);
				printf("    name:     %s\n", head->name);
				printf("    desc:     '%s'\n", head->description);
				if (head->size_specified) {
					printf("    width:    %dmm\n", head->width_mm);
					printf("    height:   %dmm\n", head->height_mm);
				} else {
					printf("    width:    (not specified)\n");
					printf("    height:   (not specified)\n");
				}
				print_head_current(head);
				break;
			case DEPARTED:
				printf("\n%s Departed:\n", head->name);
				printf("    name:     %s\n", head->name);
				printf("    desc:     '%s'\n", head->description);
				break;
			case DELTA:
				printf("%s Changing:\n  from:\n", head->name);
				print_head_current(head);
				printf("  to:\n");
				print_head_desired(head);
				break;
			default:
				break;
		}
	}

	fflush(stdout);
}

