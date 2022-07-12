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

void send_ipc_response(void) {
	if (!ipc_response) {
		return;
	}

	ipc_response_send(ipc_response);

	if (ipc_response->done) {
		log_capture_stop();
		log_capture_clear();

		close(ipc_response->fd);

		free_ipc_response(ipc_response);
		ipc_response = NULL;
	}
}

void handle_ipc(int fd_sock) {

	free_ipc_response(ipc_response);

	log_capture_clear();
	log_capture_start();

	struct IpcRequest *ipc_request = ipc_request_receive(fd_sock);
	if (!ipc_request) {
		log_error("\nFailed to read IPC request");
		log_capture_stop();
		log_capture_clear();
		return;
	}

	ipc_response = (struct IpcResponse*)calloc(1, sizeof(struct IpcResponse));
	ipc_response->done = true;

	if (ipc_request->bad) {
		ipc_response->rc = IPC_RC_BAD_REQUEST;
		goto send;
	}

	log_info("\nServer received request: %s", ipc_request_command_friendly(ipc_request->command));
	if (ipc_request->cfg) {
		print_cfg(INFO, ipc_request->cfg, ipc_request->command == CFG_DEL);
	}

	switch (ipc_request->command) {
		case CFG_DEL:
		case CFG_SET:
			{
				struct Cfg *cfg_merged = cfg_merge(cfg, ipc_request->cfg, ipc_request->command == CFG_DEL);
				if (cfg_merged) {
					// ongoing
					ipc_response->done = false;
					cfg_free(cfg);
					cfg = cfg_merged;
					log_info("\nNew configuration:");
					print_cfg(INFO, cfg, false);
				} else {
					// complete
					log_info("\nNo changes to make.");
				}
				break;
			}
		case CFG_WRITE:
			{
				// complete
				cfg_file_write();
				log_info("\nWrote configuration file: %s", cfg->file_path);
				break;
			}
		case GET:
		default:
			{
				// complete
				log_info("\nActive configuration:");
				print_cfg(INFO, cfg, false);
				print_heads(INFO, NONE, heads);
				break;
			}
	}

send:
	ipc_response->fd = ipc_request->fd;

	free_ipc_request(ipc_request);

	send_ipc_response();
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


		// inform the client
		if (ipc_response) {
			ipc_response->done = displ->config_state == IDLE;
			send_ipc_response();
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
	log_suppress_stop();
	log_capture_stop();
	log_capture_playback();
	log_capture_clear();

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

