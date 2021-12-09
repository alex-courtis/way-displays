#include "info.h"

#include "calc.h"
#include "log.h"
#include "types.h"

void print_mode(void (*log_fn)(const char*, ...), struct Mode *mode) {
	if (!log_fn || !mode)
		return;

	// TODO AMC revert back to Hz
	log_fn("    mode:     %dx%d@%ldmHz %s",
			mode->width,
			mode->height,
			// (long)(((double)mode->refresh_mHz / 1000 + 0.5)),
			mode->refresh_mHz,
			mode->preferred ? "(preferred)" : "           "
		  );
}

void print_head_current(struct Head *head) {
	if (!head)
		return;

	if (head->enabled) {
		log_info("    scale:    %.3f", wl_fixed_to_double(head->scale));
		log_info("    position: %d,%d", head->x, head->y);
		if (head->current_mode) {
			print_mode(log_info, head->current_mode);
		} else {
			log_info("    (no mode)");
		}
	} else {
		log_info("    (disabled)");
	}

	if (head->lid_closed) {
		log_info("    (lid closed)");
	}
}

void print_head_desired(struct Head *head) {
	if (!head)
		return;

	if (head->desired.enabled) {
		if (head->pending.scale) {
			log_info("    scale:    %.3f%s",
					wl_fixed_to_double(head->desired.scale),
					(!head->width_mm || !head->height_mm) ? " (default, size not specified)" : ""
				  );
		}
		if (head->pending.position) {
			log_info("    position: %d,%d",
					head->desired.x,
					head->desired.y
				  );
		}
		if (head->pending.mode && head->desired.mode) {
			print_mode(log_info, head->desired.mode);
		}
		if (head->pending.enabled && head->desired.enabled) {
			log_info("    (enabled)");
		}
	} else {
		log_info("    (disabled)");
	}
}

void print_heads(enum event event, struct SList *heads) {
	struct Head *head;
	struct SList *i, *j;

	for (i = heads; i; i = i->nex) {
		head = i->val;
		if (!head)
			continue;

		switch (event) {
			case ARRIVED:
				log_info("\n%s Arrived:", head->name);
				log_info("  info:");
				log_info("    name:     '%s'", head->name);
				log_info("    desc:     '%s'", head->description);
				if (head->width_mm && head->height_mm) {
					log_info("    width:    %dmm", head->width_mm);
					log_info("    height:   %dmm", head->height_mm);
					if (head->preferred_mode) {
						log_info("    dpi:      %.2f @ %dx%d", calc_dpi(head->preferred_mode), head->preferred_mode->width, head->preferred_mode->height);
					}
				} else {
					log_info("    width:    (not specified)");
					log_info("    height:   (not specified)");
				}
				for (j = head->modes; j; j = j->nex) {
					print_mode(log_debug, j->val);
				}
				log_info("  current:");
				print_head_current(head);
				break;
			case DEPARTED:
				log_info("\n%s Departed:", head->name);
				log_info("    name:     '%s'", head->name);
				log_info("    desc:     '%s'", head->description);
				break;
			case DELTA:
				if (is_pending_head(head)) {
					log_info("\n%s Changing:", head->name);
					log_info("  from:");
					print_head_current(head);
					log_info("  to:");
					print_head_desired(head);
				}
				break;
			default:
				break;
		}
	}
}

