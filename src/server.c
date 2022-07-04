#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include "wl_wrappers.h"

#include "server.h"

#include "cfg.h"
#include "convert.h"
#include "displ.h"
#include "fds.h"
#include "head.h"
#include "info.h"
#include "ipc.h"
#include "layout.h"
#include "lid.h"
#include "log.h"
#include "process.h"

struct Displ *displ = NULL;
struct Lid *lid = NULL;
struct Cfg *cfg = NULL;

struct IpcResponse *ipc_response = NULL;

// returns true if processed immediately
bool handle_ipc(int fd_sock) {

	ipc_response = NULL;
	free_ipc_response(ipc_response);

	struct IpcRequest *ipc_request = ipc_request_receive(fd_sock);
	if (!ipc_request) {
		log_error("\nFailed to read IPC request");
		return true;
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
		print_cfg(INFO, ipc_request->cfg, ipc_request->command == CFG_DEL);
	}

	log_capture_start();

	struct Cfg *cfg_merged = NULL;
	switch (ipc_request->command) {
		case CFG_SET:
			cfg_merged = cfg_merge(cfg, ipc_request->cfg, SET);
			break;
		case CFG_DEL:
			cfg_merged = cfg_merge(cfg, ipc_request->cfg, DEL);
			break;
		case CFG_WRITE:
			cfg_file_write();
			break;
		case CFG_GET:
		default:
			// return the active
			break;
	}

	if (cfg->written) {
		log_info("\nWrote configuration file: %s", cfg->file_path);
	}

	if (cfg_merged) {
		cfg_free(cfg);
		cfg = cfg_merged;
		log_info("\nApplying new configuration:");
	} else {
		ipc_response->done = true;
	}
	ipc_response->cfg = cfg;

	print_cfg(INFO, cfg, false);

	if (ipc_request->command == CFG_GET) {
		print_heads(INFO, NONE, heads);
	}

end:
	ipc_response->fd = ipc_request->fd;

	free_ipc_request(ipc_request);

	ipc_response_send(ipc_response);

	if (ipc_response->done) {
		free_ipc_response(ipc_response);
		ipc_response = NULL;
		return true;
	} else {
		return false;
	}
}

void finish_ipc(void) {
	if (!ipc_response) {
		return;
	}

	ipc_response->done = true;

	ipc_response_send(ipc_response);

	free_ipc_response(ipc_response);
	ipc_response = NULL;
}

// see Wayland Protocol docs Appendix B wl_display_prepare_read_queue
int loop(void) {

	for (;;) {
		init_pfds();


		// prepare for reading wayland events
		while (_wl_display_prepare_read(displ->display, FL) != 0) {
			_wl_display_dispatch_pending(displ->display, FL);
		}
		_wl_display_flush(displ->display, FL);


		// poll for all events
		if (poll(pfds, npfds, -1) < 0) {
			log_error_errno("\npoll failed, exiting");
			exit_fail();
		}


		// always read and dispatch wayland events; stop the file descriptor from getting stale
		_wl_display_read_events(displ->display, FL);
		_wl_display_dispatch_pending(displ->display, FL);
		if (!displ->output_manager) {
			log_info("\nDisplay's output manager has departed, exiting");
			exit(EXIT_SUCCESS);
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
			if (cfg_file_modified(cfg->file_name)) {
				if (cfg->written) {
					cfg->written = false;
				} else {
					cfg_file_reload();
				}
			}
		}


		// libinput lid event
		if (pfd_lid && pfd_lid->revents & pfd_lid->events) {
			lid_update();
		}


		// ipc client message
		if (pfd_ipc && (pfd_ipc->revents & pfd_ipc->events)) {
			handle_ipc(fd_ipc);
		}


		// maybe make some changes
		layout();


		// reply to the client when we are done
		if (displ->config_state == IDLE) {
			finish_ipc();
		};


		destroy_pfds();
	}
}

int
server(void) {
	log_set_times(true);

	// only one instance
	pid_file_create();

	// don't log anything until cfg log level is known
	log_capture_start();
	log_suppress_start();

	log_info("way-displays version %s", VERSION);

	// maybe default, never exits
	cfg_init();

	// play back captured logs from cfg parse
	log_set_threshold(cfg->log_threshold, false);
	log_suppress_end();
	log_capture_playback();
	log_capture_reset();

	// discover the lid state immediately
	lid_init();
	lid_update();

	// discover the output manager; it will call back
	displ_init();

	// only stops when signalled or display goes away
	int sig = loop();

	// release what remote resources we can
	heads_destroy();
	lid_destroy();
	cfg_destroy();
	displ_destroy();

	return sig;
}

