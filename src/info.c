#include <stdio.h>

#include "info.h"

#include "calc.h"
#include "types.h"

void print_mode(struct Mode *mode) {
	printf("    mode:     %dx%d@%ldHz %s\n",
			mode->width,
			mode->height,
			(long)(((double)mode->refresh_mHz / 1000 + 0.5)),
			mode->preferred ? "(preferred)" : "           "
		  );
}

void print_head_current(struct Head *head) {
	if (!head)
		return;

	if (head->enabled) {
		printf("    scale:    %.2f\n    position: %d,%d\n",
				wl_fixed_to_double(head->scale),
				head->x,
				head->y
			  );
		if (head->current_mode) {
			print_mode(head->current_mode);
		} else {
			printf("    (no mode)\n");
		}
	} else {
		printf("    (disabled)\n");
	}

	if (head->lid_closed) {
		printf("    (lid closed)\n");
	}
}

void print_head_desired(struct Head *head) {
	if (!head)
		return;

	if (head->desired.enabled) {
		if (head->pending.scale) {
			printf("    scale:    %.2f",
					wl_fixed_to_double(head->desired.scale)
				  );
			if (!head->width_mm || !head->height_mm) {
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
			print_mode(head->desired.mode);
		}
		if (head->pending.enabled && head->desired.enabled) {
			printf("    (enabled)\n");
		}
	} else {
		printf("    (disabled)\n");
	}
}

void print_heads(enum event event, struct SList *heads) {
	struct Head *head;
	struct SList *i;

	for (i = heads; i; i = i->nex) {
		head = i->val;
		if (!head)
			continue;

		switch (event) {
			case ARRIVED:
				printf("\n%s Arrived:\n", head->name);
				printf("  info:\n");
				printf("    name:     '%s'\n", head->name);
				printf("    desc:     '%s'\n", head->description);
				if (head->width_mm && head->height_mm) {
					printf("    width:    %dmm\n", head->width_mm);
					printf("    height:   %dmm\n", head->height_mm);
					if (head->preferred_mode) {
						printf("    dpi:      %.2f @ %dx%d\n", calc_dpi(head->preferred_mode), head->preferred_mode->width, head->preferred_mode->height);
					}
				} else {
					printf("    width:    (not specified)\n");
					printf("    height:   (not specified)\n");
				}
				printf("  current:\n");
				print_head_current(head);
				break;
			case DEPARTED:
				printf("\n%s Departed:\n", head->name);
				printf("    name:     '%s'\n", head->name);
				printf("    desc:     '%s'\n", head->description);
				break;
			case DELTA:
				if (is_pending_head(head)) {
					printf("\n%s Changing:\n  from:\n", head->name);
					print_head_current(head);
					printf("  to:\n");
					print_head_desired(head);
				}
				break;
			default:
				break;
		}
	}

	fflush(stdout);
}

