#include <poll.h>
#include <string.h>
#include <sysexits.h>

#include "laptop.h"
#include "listeners.h"
#include "types.h"
#include "util.h"
#include "wl_wrappers.h"

void print_om(struct OutputManager *output_manager) {
	fprintf(stderr, "print_om %p\n", (void*)output_manager);
	if (!output_manager)
		return;

	struct Head *head;
	for (struct SList *i = output_manager->heads; i; i = i->nex) {
		head = i->val;
		fprintf(stderr, "%s '%s' %dmm x %dmm%s %d,%d %d %f '%s' '%s' '%s'", head->name, head->description, head->width_mm, head->height_mm, head->enabled ? " (enabled) " : " (disabled)", head->x, head->y, head->transform, wl_fixed_to_double(head->scale), head->make, head->model, head->serial_number);

		if (head->current_mode) {
			fprintf(stderr," %dx%d@%d %p %s", head->current_mode->width, head->current_mode->height, head->current_mode->refresh_mHz, (void*)head->current_mode->zwlr_mode, head->current_mode->preferred ? "(preferred)" : "");
		}
		fprintf(stderr, "\n");
		/* struct Mode *mode; */
		/* for (struct SList *i = head->modes; i; i = i->nex) { */
		/* 	mode = i->val; */
		/* 	fprintf(stderr, "%dx%d@%d %p %s\n", mode->width, mode->height, mode->refresh_mHz, (void*)mode->zwlr_mode, mode->preferred ? "(preferred)" : ""); */
		/* } */
	}
}

void ltr_arrange(struct OutputManager *output_manager) {
	fprintf(stderr, "ltr_arrange\n");
	struct Head *head;
	for (struct SList *i = output_manager->heads; i; i = i->nex) {
		head = (struct Head*)i->val;
		head->desired.enabled = !closed_laptop_display(head->name);
		head->pending.enabled = true;
		head->desired.mode = optimal_mode(head->modes);
		head->pending.mode = true;
		head->desired.scale = auto_scale(head);
		head->pending.scale = true;
	}

	struct SList *order = NULL;
	char *order1 = strdup("DP-3");
	slist_append(&order, order1);
	char *order2 = strdup("eDP-1");
	slist_append(&order, order2);

	slist_free(&output_manager->desired.heads_ordered);
	output_manager->desired.heads_ordered = order_heads(order, output_manager->heads);

	ltr_heads(output_manager->desired.heads_ordered);
}

void apply_desired(struct OutputManager *output_manager) {
	fprintf(stderr, "apply_desired\n");
	struct Head *head;

	struct zwlr_output_configuration_v1 *zwlr_config = zwlr_output_manager_v1_create_configuration(output_manager->zwlr_output_manager, output_manager->serial);
	zwlr_output_configuration_v1_add_listener(zwlr_config, output_configuration_listener(), output_manager);
	for (struct SList *i = output_manager->desired.heads_ordered; i; i = i->nex) {
		head = (struct Head*)i->val;
		if (head->desired.enabled) {
			fprintf(stderr, "apply_desired enabling, positioning at %d,%d scaling to %f and setting mode for %s\n", head->desired.x, head->desired.y, wl_fixed_to_double(head->desired.scale), head->name);

			struct zwlr_output_configuration_head_v1 *config_head = zwlr_output_configuration_v1_enable_head(zwlr_config, head->zwlr_head);
			zwlr_output_configuration_head_v1_set_mode(config_head, head->desired.mode->zwlr_mode);
			zwlr_output_configuration_head_v1_set_scale(config_head, head->desired.scale);

			/* zwlr_output_configuration_head_v1_set_scale(config_head, 0); */
			zwlr_output_configuration_head_v1_set_position(config_head, head->desired.x, head->desired.y);
		} else {
			fprintf(stderr, "apply_desired disabling %s\n", head->name);

			zwlr_output_configuration_v1_disable_head(zwlr_config, head->zwlr_head);
		}
	}

	zwlr_output_configuration_v1_apply(zwlr_config);
	fprintf(stderr, "apply_desired done\n");
}

// see Wayland Protocol docs Appendix B wl_display_prepare_read_queue
void listen(struct Displ *displ) {
	struct pollfd readfds[1] = {0};
	readfds[0].fd = wl_display_get_fd(displ->display);
	readfds[0].events = POLLIN;

	int num_pending = 0;
	int loops = 0;
	int nloops = 8;
	for (;;) {
		fprintf(stderr, "\n\nlisten %d\n", loops);

		while (checked_wl_display_prepare_read(displ->display) != 0) {
			num_pending = checked_wl_display_dispatch_pending(displ->display);
			fprintf(stderr, "listen 1 dispatched %d pending\n", num_pending);
		}

		checked_wl_display_flush(displ->display);

		fprintf(stderr, "listen polling\n");
		if (poll(readfds, 1, -1) > 0) {
			checked_wl_display_read_events(displ->display);
		} else {
			checked_wl_display_cancel_read(displ->display);
		}


		num_pending = checked_wl_display_dispatch_pending(displ->display);
		fprintf(stderr, "listen 2 dispatched %d pending\n", num_pending);


		if (!displ->output_manager) {
			fprintf(stderr, "ERROR: output manager has been destroyed, exiting\n");
			exit(EX_SOFTWARE);
		}


		print_om(displ->output_manager);


		if (is_dirty(displ->output_manager) && !is_pending(displ->output_manager)) {
			fprintf(stderr, "listen dirty, arranging\n");

			reset_dirty(displ->output_manager);

			ltr_arrange(displ->output_manager);
			apply_desired(displ->output_manager);
		} else {
			fprintf(stderr, "listen nothingtodohere\n");
		}


		loops++;
		if (loops == (nloops - 2)) {
			fprintf(stderr, "listen disconnecting WLR\n");
			zwlr_output_manager_v1_stop(displ->output_manager->zwlr_output_manager);
			fprintf(stderr, "listen disconnected WLR\n");
			continue;
		}

		if (loops >= nloops) {
			break;
		}


		fprintf(stderr, "listen end\n");
	}

	fprintf(stderr, "listen exited after %d loops\n", loops);
}

int
main(int argc, const char **argv) {

	struct Displ *displ = calloc(1, sizeof(struct Displ));

	displ->display = checked_wl_display_connect(NULL);

	struct wl_registry *registry = wl_display_get_registry(displ->display);
	wl_registry_add_listener(registry, registry_listener(), displ);

	listen(displ);

	// TODO is this ever necessary? only if we can exit the loop
	fprintf(stderr, "disconnecting WL\n");
	wl_display_disconnect(displ->display);
	fprintf(stderr, "disconnected WL\n");

	free_displ(displ);

	return EXIT_SUCCESS;
}

