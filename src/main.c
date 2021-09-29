#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>

#include "cfg.h"
#include "info.h"
#include "lid.h"
#include "listeners.h"
#include "layout.h"
#include "types.h"
#include "wl_wrappers.h"

void connect_display(struct Displ *displ) {
	struct wl_registry *registry;

	if (!(displ->display = wl_display_connect(NULL))) {
		fprintf(stderr, "\nERROR: Unable to connect to the compositor. Check or set the WAYLAND_DISPLAY environment variable. exiting\n");
		exit(EX_SOFTWARE);
	}

	registry = wl_display_get_registry(displ->display);

	wl_registry_add_listener(registry, registry_listener(), displ);

	if (wl_display_roundtrip(displ->display) == -1) {
		fprintf(stderr, "\nERROR: wl_display_roundtrip failed -1, exiting");
		exit(EX_SOFTWARE);
	}

	if (!displ->output_manager) {
		fprintf(stderr, "\nERROR: compositor does not support WLR output manager protocol, exiting\n");
		exit(EX_SOFTWARE);
	}
}

enum {
	PFD_WL = 0,
	PFD_LI,
	PFD_MAX,
};

void create_pfds(struct Displ *displ) {
	displ->npfds = displ->lid ? PFD_MAX : PFD_MAX - 1;
	displ->pfds = calloc(displ->npfds, sizeof(struct pollfd));
	displ->pfds[PFD_WL].fd = wl_display_get_fd(displ->display);
	displ->pfds[PFD_WL].events = POLLIN;
	if (displ->lid) {
		displ->pfds[PFD_LI].fd = displ->lid->libinput_fd;
		displ->pfds[PFD_LI].events = POLLIN;
	}
}

// see Wayland Protocol docs Appendix B wl_display_prepare_read_queue
void listen(struct Displ *displ) {
	bool heads_arrived_or_departed;

	for (;;) {

		// prepare for reading wayland events
		while (_wl_display_prepare_read(displ->display, FL) != 0) {
			_wl_display_dispatch_pending(displ->display, FL);
		}
		_wl_display_flush(displ->display, FL);


		// poll for wayland and libinput events
		if (poll(displ->pfds, displ->npfds, -1) < 0) {
			fprintf(stderr, "\nERROR: poll failed %d: '%s', exiting\n", errno, strerror(errno));
		}


		// safe to always read and dispatch wayland events
		_wl_display_read_events(displ->display, FL);
		_wl_display_dispatch_pending(displ->display, FL);


		if (!displ->output_manager) {
			printf("\nDisplay's output manager has departed, exiting\n");
			exit(EXIT_SUCCESS);
		}


		// dispatch libinput events only when we have received a change
		if (displ->npfds > PFD_LI && displ->pfds[PFD_LI].revents & displ->pfds[PFD_LI].events) {
			update_lid(displ);
		}

		// always do this, for the case of new arrivals
		update_heads_lid_closed(displ);


		// inform of arrivals and departures, usually a NOP
		heads_arrived_or_departed = displ->output_manager->heads_arrived || displ->output_manager->heads_departed;
		print_heads(ARRIVED, displ->output_manager->heads_arrived);
		output_manager_release_heads_arrived(displ->output_manager);
		print_heads(DEPARTED, displ->output_manager->heads_departed);
		output_manager_free_heads_departed(displ->output_manager);


		// if we have no changes in progress we can maybe make changes
		if (is_dirty(displ) && !is_pending_output_manager(displ->output_manager)) {

			// prepare possible changes
			reset_dirty(displ);
			desire_ltr(displ);
			pend_desired(displ);

			if (is_pending_output_manager(displ->output_manager)) {

				// inform and apply
				print_heads(DELTA, displ->output_manager->heads);
				apply_desired(displ);

			} else if (heads_arrived_or_departed) {
				printf("\nNo changes needed\n");
			}
		}
	}
}

int
main(int argc, const char **argv) {
	struct Displ *displ = calloc(1, sizeof(struct Displ));

	// always returns a cfg, possibly default
	displ->cfg = read_cfg();

	// also informs of defaults
	print_cfg(displ->cfg);

	// discover the output manager via a roundtrip
	connect_display(displ);

	// this may be null
	displ->lid = create_lid();

	// update once to discover initial switch state
	update_lid(displ);

	// polling fds for wayland and maybe libinput
	create_pfds(displ);

	listen(displ);

	// unreachable
}

