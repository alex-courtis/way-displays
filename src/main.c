#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include "cfg.h"
#include "displ.h"
#include "fds.h"
#include "info.h"
#include "lid.h"
#include "log.h"
#include "layout.h"
#include "process.h"
#include "types.h"
#include "wl_wrappers.h"

// see Wayland Protocol docs Appendix B wl_display_prepare_read_queue
int listen(struct Displ *displ) {
	bool user_changes = false;
	bool initial_run_complete = false;
	bool lid_discovery_complete = false;

	for (;;) {
		user_changes = false;
		create_pfds(displ);


		// prepare for reading wayland events
		while (_wl_display_prepare_read(displ->display, FL) != 0) {
			_wl_display_dispatch_pending(displ->display, FL);
		}
		_wl_display_flush(displ->display, FL);


		if (!initial_run_complete || lid_discovery_complete) {
			// poll for signal, wayland and maybe libinput, cfg file events
			if (poll(pfds, npfds, -1) < 0) {
				log_error("\npoll failed %d: '%s', exiting", errno, strerror(errno));
			}
		} else {
			// takes ~1 sec hence we defer
			displ->lid = create_lid();
			update_lid(displ);
			lid_discovery_complete = true;
		}


		// subscribed signals are all a clean exit
		if (pfd_signal && pfd_signal->revents & pfd_signal->events) {
			struct signalfd_siginfo fdsi;
			if (read(fd_signal, &fdsi, sizeof(fdsi)) == sizeof(fdsi)) {
				return fdsi.ssi_signo;
			}
		}


		// cfg directory change
		if (pfd_cfg_dir && pfd_cfg_dir->revents & pfd_cfg_dir->events) {
			if (cfg_file_written(displ->cfg->file_name)) {
				user_changes = true;
				displ->cfg = reload_cfg(displ->cfg);
			}
		}


		// safe to always read and dispatch wayland events
		_wl_display_read_events(displ->display, FL);
		_wl_display_dispatch_pending(displ->display, FL);


		if (!displ->output_manager) {
			log_info("\nDisplay's output manager has departed, exiting");
			exit(EXIT_SUCCESS);
		}


		// dispatch libinput events only when we have received a change
		if (pfd_lid && pfd_lid->revents & pfd_lid->events) {
			user_changes = user_changes || update_lid(displ);
		}
		// always do this, to cover the initial case
		update_heads_lid_closed(displ);


		// inform of head arrivals and departures and clean them
		user_changes = user_changes || consume_arrived_departed(displ->output_manager);


		// if we have no changes in progress we can maybe react to inital or modified state
		if (is_dirty(displ) && !is_pending_output_manager(displ->output_manager)) {

			// prepare possible changes
			reset_dirty(displ);
			desire_ltr(displ);
			pend_desired(displ);

			if (is_pending_output_manager(displ->output_manager)) {

				// inform and apply
				print_heads(DELTA, displ->output_manager->heads);
				apply_desired(displ);

			} else if (user_changes) {
				log_info("\nNo changes needed");
			}
		}


		// no changes are outstanding
		if (!is_pending_output_manager(displ->output_manager)) {
			initial_run_complete = true;
		}


		destroy_pfds();
	}
}

int
main(int argc, const char **argv) {
	setlinebuf(stdout);

	struct Displ *displ = calloc(1, sizeof(struct Displ));

	log_info("way-displays version %s", VERSION);

	// only one instance
	ensure_singleton();

	// always returns a cfg, possibly default
	displ->cfg = load_cfg();

	// also informs of defaults
	print_cfg(displ->cfg);

	// discover the output manager via a roundtrip
	connect_display(displ);

	// only stops when signalled or display goes away
	int sig = listen(displ);

	// release what remote resources we can
	destroy_display(displ);

	return sig;
}

