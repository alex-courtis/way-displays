#include <stdio.h>
#include <stdlib.h>

#include "listeners.h"
#include "types.h"

int
main(int argc, const char **argv) {

	struct wl_display *display = wl_display_connect(NULL);
	if (display == NULL) {
		fprintf(stderr, "failed to connect to display\n");
		return EXIT_FAILURE;
	}

	struct OutputManager *output_manager = calloc(1, sizeof(*output_manager));
	wl_list_init(&output_manager->heads);

	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, registry_listener(), output_manager);

	wl_display_dispatch(display);

	wl_display_roundtrip(display);

	// todo: work out how to release the display/listener for cleanup

	// todo: handle compositior not supporting wlr-output-management-unstable-v1

	struct Head *head;
	wl_list_for_each(head, &output_manager->heads, link) {
		printf("%s '%s' %dmm x %dmm%s %d,%d %d %f %s %s %s\n", head->name, head->description, head->width_mm, head->height_mm, head->enabled ? " (enabled)" : "", head->x, head->y, head->transform, wl_fixed_to_double(head->scale), head->make, head->model, head->serial_number);
		struct Mode *mode;
		wl_list_for_each(mode, &head->modes, link) {
			printf("%dx%d@%dHz%s, ", mode->width, mode->height, (mode->refresh + 500) / 1000, mode->preferred ? "(preferred)" : "");
		}
		printf("\n\n");
	}

	struct zwlr_output_configuration_v1 *zwlr_config = zwlr_output_manager_v1_create_configuration(output_manager->zwlr_output_manager, output_manager->serial);
	// TODO: this listener is never invoked
	zwlr_output_configuration_v1_add_listener(zwlr_config, output_configuration_listener(), 0);
	wl_list_for_each(head, &output_manager->heads, link) {
		printf("AMC applying scale 3 to %p\n", (void*)head->zwlr_head);
		struct zwlr_output_configuration_head_v1 *config_head = zwlr_output_configuration_v1_enable_head(zwlr_config, head->zwlr_head);
		zwlr_output_configuration_head_v1_set_scale(config_head, wl_fixed_from_double(4));
	}

	// TODO: something with this result?
	/* zwlr_output_configuration_v1_test(zwlr_config); */

	zwlr_output_configuration_v1_apply(zwlr_config);

	wl_display_flush(display);

	return EXIT_SUCCESS;
}

