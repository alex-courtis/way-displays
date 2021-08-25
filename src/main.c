#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "listeners.h"
#include "types.h"
#include "util.h"

int
main(int argc, const char **argv) {

	struct wl_display *display = wl_display_connect(NULL);
	if (display == NULL) {
		fprintf(stderr, "failed to connect to display\n");
		return EXIT_FAILURE;
	}

	struct OutputManager *output_manager = calloc(1, sizeof(struct OutputManager));
	wl_list_init(&output_manager->heads);
	wl_list_init(&output_manager->desired.heads_enabled);
	wl_list_init(&output_manager->desired.heads_disabled);
	wl_list_init(&output_manager->desired.order);

	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, registry_listener(), output_manager);

	wl_display_dispatch(display);

	wl_display_roundtrip(display);

	// TODO work out how to release the display/listener for cleanup

	// TODO handle compositior not supporting wlr-output-management-unstable-v1

	struct Head *head;
	wl_list_for_each(head, &output_manager->heads, link) {
		printf("%s '%s' %dmm x %dmm%s %d,%d %d %f '%s' '%s' '%s'\n", head->name, head->description, head->width_mm, head->height_mm, head->enabled ? " (enabled) " : " (disabled)", head->x, head->y, head->transform, wl_fixed_to_double(head->scale), head->make, head->model, head->serial_number);
		struct Mode *mode;
		wl_list_for_each(mode, &head->modes, link) {
			printf("%dx%d@%d %p %s\n", mode->width, mode->height, mode->refresh_mHz, (void*)mode->zwlr_mode, mode->preferred ? "(preferred)" : "");
		}
	}

	struct HeadOrder *order = calloc(1, sizeof(struct HeadOrder));
	order->name_desc = strdup("DP-3");
	wl_list_insert(&output_manager->desired.order, &order->link);

	order = calloc(1, sizeof(struct HeadOrder));
	order->name_desc = strdup("eDP-1");
	wl_list_insert(&output_manager->desired.order, &order->link);

	wl_list_for_each(head, &output_manager->heads, link) {
		head->desired.enabled = true;
		head->desired.mode = optimal_mode(&head->modes);
		head->desired.scale = auto_scale(head);
	}
	order_desired_heads(output_manager);

	struct zwlr_output_configuration_v1 *zwlr_config = zwlr_output_manager_v1_create_configuration(output_manager->zwlr_output_manager, output_manager->serial);
	zwlr_output_configuration_v1_add_listener(zwlr_config, output_configuration_listener(), 0);
	wl_list_for_each(head, &output_manager->desired.heads_enabled, link) {
		printf("enabling, scaling and setting mode for %s\n", head->name);

		struct zwlr_output_configuration_head_v1 *config_head = zwlr_output_configuration_v1_enable_head(zwlr_config, head->zwlr_head);
		zwlr_output_configuration_head_v1_set_mode(config_head, head->desired.mode->zwlr_mode);
		zwlr_output_configuration_head_v1_set_scale(config_head, head->desired.scale);
	}

	wl_list_for_each(head, &output_manager->desired.heads_disabled, link) {
		printf("disabling %s\n", head->name);

		zwlr_output_configuration_v1_disable_head(zwlr_config, head->zwlr_head);
	}

	// TODO something with this result?
	/* zwlr_output_configuration_v1_test(zwlr_config); */

	zwlr_output_configuration_v1_apply(zwlr_config);

	wl_display_flush(display);

	fflush(stdout);

	return EXIT_SUCCESS;
}

