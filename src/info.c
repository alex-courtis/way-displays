#include "info.h"

#include "calc.h"
#include "log.h"
#include "types.h"

void print_mode(struct Mode *mode) {
	log_info("    mode:     %dx%d@%ldHz %s\n",
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
		log_info("    scale:    %.3f\n", wl_fixed_to_double(head->scale));
		log_info("    position: %d,%d\n", head->x, head->y);
		if (head->current_mode) {
			print_mode(head->current_mode);
		} else {
			log_info("    (no mode)\n");
		}
	} else {
		log_info("    (disabled)\n");
	}

	if (head->lid_closed) {
		log_info("    (lid closed)\n");
	}
}

void print_head_desired(struct Head *head) {
	if (!head)
		return;

	if (head->desired.enabled) {
		if (head->pending.scale) {
			log_info("    scale:    %.3f%s\n",
					wl_fixed_to_double(head->desired.scale),
					(!head->width_mm || !head->height_mm) ? " (default, size not specified)" : ""
				  );
		}
		if (head->pending.position) {
			log_info("    position: %d,%d\n",
					head->desired.x,
					head->desired.y
				  );
		}
		if (head->pending.mode && head->desired.mode) {
			print_mode(head->desired.mode);
		}
		if (head->pending.enabled && head->desired.enabled) {
			log_info("    (enabled)\n");
		}
	} else {
		log_info("    (disabled)\n");
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
				log_info("\n%s Arrived:\n", head->name);
				log_info("  info:\n");
				log_info("    name:     '%s'\n", head->name);
				log_info("    desc:     '%s'\n", head->description);
				if (head->width_mm && head->height_mm) {
					log_info("    width:    %dmm\n", head->width_mm);
					log_info("    height:   %dmm\n", head->height_mm);
					if (head->preferred_mode) {
						log_info("    dpi:      %.2f @ %dx%d\n", calc_dpi(head->preferred_mode), head->preferred_mode->width, head->preferred_mode->height);
					}
				} else {
					log_info("    width:    (not specified)\n");
					log_info("    height:   (not specified)\n");
				}
				log_info("  current:\n");
				print_head_current(head);
				break;
			case DEPARTED:
				log_info("\n%s Departed:\n", head->name);
				log_info("    name:     '%s'\n", head->name);
				log_info("    desc:     '%s'\n", head->description);
				break;
			case DELTA:
				if (is_pending_head(head)) {
					log_info("\n%s Changing:\n", head->name);
					log_info("  from:\n");
					print_head_current(head);
					log_info("  to:\n");
					print_head_desired(head);
				}
				break;
			default:
				break;
		}
	}
}

