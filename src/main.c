#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>

#include "cfg.h"
#include "displ.h"
#include "info.h"
#include "lid.h"
#include "listeners.h"
#include "layout.h"
#include "process.h"
#include "types.h"
#include "wl_wrappers.h"

enum {
	PFD_WL = 0,
	PFD_LI,
	PFD_MAX,
};

int npfds;
struct pollfd *pfds;

void create_pfds(struct Displ *displ) {
	npfds = displ->lid ? PFD_MAX : PFD_MAX - 1;
	pfds = calloc(npfds, sizeof(struct pollfd));
	pfds[PFD_WL].fd = wl_display_get_fd(displ->display);
	pfds[PFD_WL].events = POLLIN;
	if (displ->lid) {
		pfds[PFD_LI].fd = displ->lid->libinput_fd;
		pfds[PFD_LI].events = POLLIN;
	}
}

void destroy_pfds() {
	npfds = 0;
	free(pfds);
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
		create_pfds(displ);
		if (poll(pfds, npfds, -1) < 0) {
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
		if (npfds > PFD_LI && pfds[PFD_LI].revents & pfds[PFD_LI].events) {
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

		destroy_pfds();
	}
}

int
main(int argc, const char **argv) {
	struct Displ *displ = calloc(1, sizeof(struct Displ));

	// only one instance
	ensure_singleton();

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

	listen(displ);

	destroy_display(displ);

	return EXIT_SUCCESS;
}

