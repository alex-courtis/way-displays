#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include "server.h"

#include "cfg.h"
#include "convert.h"
#include "displ.h"
#include "fds.h"
#include "info.h"
#include "ipc.h"
#include "layout.h"
#include "lid.h"
#include "log.h"
#include "process.h"
#include "types.h"
#include "wl_wrappers.h"

struct Displ *displ = NULL;

struct IpcResponse *ipc_response = NULL;

bool user_changes = false;
bool initial_run_complete = false;
bool lid_discovery_complete = false;

void handle_ipc(int fd_sock) {

	ipc_response = NULL;
	free_ipc_response(ipc_response);

	struct IpcRequest *ipc_request = ipc_request_receive(fd_sock);
	if (!ipc_request) {
		log_error("\nFailed to read IPC request");
		return;
	}

	ipc_response = (struct IpcResponse*)calloc(1, sizeof(struct IpcResponse));
	ipc_response->rc = EXIT_SUCCESS;
	ipc_response->done = false;

	if (ipc_request->bad) {
		ipc_response->rc = EXIT_FAILURE;
		ipc_response->done = true;
		goto end;
	}

	log_info("\nServer received %s request:", ipc_request_command_friendly(ipc_request->command));
	if (ipc_request->cfg) {
		print_cfg(ipc_request->cfg);
	}

	log_capture_start();

	struct Cfg *cfg_merged = NULL;
	switch (ipc_request->command) {
		case CFG_SET:
			cfg_merged = cfg_merge(displ->cfg, ipc_request->cfg, SET);
			break;
		case CFG_DEL:
			cfg_merged = cfg_merge(displ->cfg, ipc_request->cfg, DEL);
			break;
		case CFG_WRITE:
			cfg_file_write(displ->cfg);
			break;
		case CFG_GET:
		default:
			// return the active
			break;
	}

	if (displ->cfg->written) {
		log_info("\nWrote configuration file: %s", displ->cfg->file_path);
	}

	if (cfg_merged) {
		free_cfg(displ->cfg);
		displ->cfg = cfg_merged;
		displ->cfg->dirty = true;
		log_info("\nApplying new configuration:");
	} else {
		log_info("\nActive configuration:");
		ipc_response->done = true;
	}

	log_set_threshold(displ->cfg->log_threshold, false);

	print_cfg(displ->cfg);

	if (ipc_request->command == CFG_GET) {
		print_heads(NONE, displ->output_manager->heads);
	}

end:
	ipc_response->fd = ipc_request->fd;

	free_ipc_request(ipc_request);

	ipc_response_send(ipc_response);

	if (ipc_response->done) {
		free_ipc_response(ipc_response);
		ipc_response = NULL;
	}
}

void finish_ipc() {
	if (!ipc_response) {
		return;
	}

	ipc_response->done = true;

	ipc_response_send(ipc_response);

	free_ipc_response(ipc_response);
	ipc_response = NULL;
}

void handle_changes() {

	// if we have no changes in progress we can maybe react to inital or modified state
	if (is_dirty(displ) && !is_pending_output_manager(displ->output_manager)) {

		// prepare possible changes
		reset_dirty(displ);
		desire_arrange(displ);
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
		finish_ipc();
		initial_run_complete = true;
	}
}

// see Wayland Protocol docs Appendix B wl_display_prepare_read_queue
int loop() {

	init_fds(displ->cfg);
	for (;;) {
		user_changes = false;
		create_pfds(displ);


		// prepare for reading wayland events
		while (_wl_display_prepare_read(displ->display, FL) != 0) {
			_wl_display_dispatch_pending(displ->display, FL);
		}
		_wl_display_flush(displ->display, FL);


		if (!initial_run_complete || lid_discovery_complete) {
			if (poll(pfds, npfds, -1) < 0) {
				log_error_errno("\npoll failed, exiting");
				exit(EXIT_FAILURE);
			}
		} else {
			// takes ~1 sec hence we defer
			displ->lid = create_lid();
			update_lid(displ);
			lid_discovery_complete = true;
		}


		// subscribed signals are mostly a clean exit
		if (pfd_signal && pfd_signal->revents & pfd_signal->events) {
			struct signalfd_siginfo fdsi;
			if (read(fd_signal, &fdsi, sizeof(fdsi)) == sizeof(fdsi)) {
				if (fdsi.ssi_signo != SIGPIPE) {
					return fdsi.ssi_signo;
				}
			}
		}


		// cfg directory change
		if (pfd_cfg_dir && pfd_cfg_dir->revents & pfd_cfg_dir->events) {
			if (cfg_file_modified(displ->cfg->file_name)) {
				if (displ->cfg->written) {
					displ->cfg->written = false;
				} else {
					user_changes = true;
					displ->cfg = cfg_file_reload(displ->cfg);
				}
			}
		}


		// ipc client message
		if (pfd_ipc && pfd_ipc->revents & pfd_ipc->events) {
			handle_ipc(fd_ipc);
			user_changes = (ipc_response != NULL);
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


		handle_changes();


		destroy_pfds();
	}
}

int
server() {
	log_set_times(true);

	displ = calloc(1, sizeof(struct Displ));

	log_info("way-displays version %s", VERSION);

	// only one instance
	pid_file_create();

	// always returns a cfg, possibly default
	displ->cfg = cfg_file_load();

	// discover the output manager via a roundtrip
	connect_display(displ);

	// only stops when signalled or display goes away
	int sig = loop();

	// release what remote resources we can
	destroy_display(displ);

	return sig;
}

