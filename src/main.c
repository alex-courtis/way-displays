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

	struct wl_list heads;
	wl_list_init(&heads);

	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, registry_listener(), &heads);

	wl_display_dispatch(display);

	wl_display_roundtrip(display);

	// todo: work out how to release the display/listener for cleanup

	// todo: handle compositior not supporting wlr-output-management-unstable-v1

	struct Head *head;
	wl_list_for_each(head, &heads, link) {
		printf("%s '%s' %dmm x %dmm%s %d,%d %d %f %s %s %s\n", head->name, head->description, head->width_mm, head->height_mm, head->enabled ? " (enabled)" : "", head->x, head->y, head->transform, wl_fixed_to_double(head->scale), head->make, head->model, head->serial_number);
		struct Mode *mode;
		wl_list_for_each(mode, &head->modes, link) {
			printf("%dx%d@%dHz%s, ", mode->width, mode->height, (mode->refresh + 500) / 1000, mode->preferred ? "(preferred)" : "");
		}
		printf("\n\n");
	}

	// todo: free heads

	return EXIT_SUCCESS;
}

