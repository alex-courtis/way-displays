#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "listeners.h"
#include "types.h"
#include "util.h"

struct wl_display *display;
struct OutputManager *output_manager;

void print_om(struct OutputManager *output_manager) {
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

void listen() {
	int ret;

	struct pollfd readfds[1] = {0};
	readfds[0].fd = wl_display_get_fd(display);
	readfds[0].events = POLLIN;

	int loops = 0;
	int nloops = 3;
	for (;;) {
		fprintf(stderr, "listen\n");


		print_om(output_manager);


		if (loops++ >= nloops) {
			break;
		}


		fprintf(stderr, "listen preparing read\n");
		int n = 0;
		while (wl_display_prepare_read(display) != 0) {
			if (wl_display_dispatch_pending(display) == -1) {
				exit(EXIT_FAILURE);
			}
			n++;
		}
		fprintf(stderr, "listen dispatched %d pending\n", n);


		do {
			fprintf(stderr, "listen polling\n");
			ret = poll(readfds, sizeof(readfds) / sizeof(readfds[0]), -1);
		} while (ret == -1 && errno == EINTR);
		if (ret == -1) {
			fprintf(stderr, "listen poll fail errno=%d\n", errno);
			exit(EXIT_FAILURE);
		}
		fprintf(stderr, "listen polled ret=%d\n", ret);


		fprintf(stderr, "listen wl_display_read_events\n");
		if (wl_display_read_events(display) == -1) {
			fprintf(stderr, "listen wl_display_read_events fail -1\n");
			exit(EXIT_FAILURE);
		}


		fprintf(stderr, "listen end\n");
	}

	fprintf(stderr, "listen exited after %d loops\n", nloops);
}

int
main(int argc, const char **argv) {

	display = wl_display_connect(NULL);
	if (display == NULL) {
		fprintf(stderr, "failed to connect to display\n");
		return EXIT_FAILURE;
	}

	output_manager = calloc(1, sizeof(struct OutputManager));

	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, registry_listener(), output_manager);

	wl_display_dispatch(display);

	wl_display_roundtrip(display);

	listen();

	wl_display_disconnect(display);

	return EXIT_SUCCESS;

	// TODO work out how to release the display/listener for cleanup

	// TODO handle compositior not supporting wlr-output-management-unstable-v1

	struct Head *head;
	for (struct SList *i = output_manager->heads; i; i = i->nex) {
		head = (struct Head*)i->val;
		head->desired.enabled = true;
		head->desired.mode = optimal_mode(head->modes);
		head->desired.scale = auto_scale(head);
	}

	struct SList *order = NULL;
	char *order1 = strdup("DP-3");
	slist_append(&order, order1);
	char *order2 = strdup("eDP-1");
	slist_append(&order, order2);
	output_manager->desired.heads_ordered = order_heads(order, output_manager->heads);
	ltr_heads(output_manager->desired.heads_ordered);

	struct zwlr_output_configuration_v1 *zwlr_config = zwlr_output_manager_v1_create_configuration(output_manager->zwlr_output_manager, output_manager->serial);
	zwlr_output_configuration_v1_add_listener(zwlr_config, output_configuration_listener(), 0);
	for (struct SList *i = output_manager->desired.heads_ordered; i; i = i->nex) {
		head = (struct Head*)i->val;
		if (head->desired.enabled) {
			fprintf(stderr, "enabling, positioning at %d,%d scaling to %f and setting mode for %s\n", head->desired.x, head->desired.y, wl_fixed_to_double(head->desired.scale), head->name);

			struct zwlr_output_configuration_head_v1 *config_head = zwlr_output_configuration_v1_enable_head(zwlr_config, head->zwlr_head);
			zwlr_output_configuration_head_v1_set_mode(config_head, head->desired.mode->zwlr_mode);
			zwlr_output_configuration_head_v1_set_scale(config_head, head->desired.scale);
			zwlr_output_configuration_head_v1_set_position(config_head, head->desired.x, head->desired.y);
		} else {
			fprintf(stderr, "disabling %s\n", head->name);

			zwlr_output_configuration_v1_disable_head(zwlr_config, head->zwlr_head);
		}
	}

	// TODO something with this result?
	/* zwlr_output_configuration_v1_test(zwlr_config); */

	zwlr_output_configuration_v1_apply(zwlr_config);

	return EXIT_SUCCESS;
}

