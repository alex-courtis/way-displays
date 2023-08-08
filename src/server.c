#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include "wl_wrappers.h"

#include "server.h"

#include "cfg.h"
#include "convert.h"
#include "displ.h"
#include "fds.h"
#include "global.h"
#include "head.h"
#include "info.h"
#include "ipc.h"
#include "layout.h"
#include "lid.h"
#include "log.h"
#include "process.h"

// operation in progress
struct IpcOperation *ipc_operation = NULL;

// received a request whilst another is in progress
void handle_ipc_in_progress(int server_socket) {
	struct IpcRequest *request = ipc_receive_request(server_socket);
	if (!request) {
		log_error("\nFailed to read IPC request");
		return;
	}

	struct IpcOperation *operation = (struct IpcOperation*)calloc(1, sizeof(struct IpcOperation));
	operation->request = request;
	operation->socket_client = request->socket_client;
	operation->done = true;
	operation->rc = IPC_RC_REQUEST_IN_PROGRESS;

	ipc_send_operation(operation);

	close(operation->socket_client);

	ipc_operation_free(operation);
}

void notify_ipc_operation(void) {
	if (!ipc_operation) {
		return;
	}

	ipc_send_operation(ipc_operation);

	if (ipc_operation->done) {
		log_capture_stop();
		log_capture_clear();

		close(ipc_operation->socket_client);

		ipc_operation_free(ipc_operation);
		ipc_operation = NULL;
	}
}

void receive_ipc_request(int server_socket) {
	if (ipc_operation) {
		handle_ipc_in_progress(server_socket);
		return;
	}

	log_capture_clear();
	log_capture_start();

	struct IpcRequest *ipc_request = ipc_receive_request(server_socket);
	if (!ipc_request) {
		log_error("\nFailed to read IPC request");
		log_capture_stop();
		log_capture_clear();
		return;
	}

	ipc_operation = (struct IpcOperation*)calloc(1, sizeof(struct IpcOperation));
	ipc_operation->request = ipc_request;
	ipc_operation->socket_client = ipc_request->socket_client;
	ipc_operation->done = true;
	ipc_operation->send_logs = true;
	ipc_operation->send_state = true;

	if (ipc_request->bad) {
		ipc_operation->rc = IPC_RC_BAD_REQUEST;
		ipc_operation->send_state = false;
		goto send;
	}

	log_info("\nServer received request: %s", ipc_command_friendly(ipc_request->command));
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
					ipc_operation->done = false;
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
				break;
			}
		case GET:
		default:
			{
				// complete
				log_info("\nActive configuration:");
				print_cfg(INFO, cfg, false);
				print_cfg_commands(INFO, cfg);
				print_heads(INFO, NONE, heads);
				break;
			}
	}

send:
	notify_ipc_operation();
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
			wd_exit_message(EXIT_FAILURE);
			return EXIT_FAILURE;
		}


		// always read and dispatch wayland events; stop the file descriptor from getting stale
		_wl_display_read_events(displ->display, FL);
		_wl_display_dispatch_pending(displ->display, FL);
		if (!displ->output_manager) {
			log_info("\nDisplay's output manager has departed, exiting");
			wd_exit(EXIT_SUCCESS);
			return EXIT_SUCCESS;
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
			receive_ipc_request(fd_socket_server);
		}


		// maybe make some changes
		layout();


		// inform the client
		if (ipc_operation) {
			ipc_operation->done = displ->config_state == IDLE;
			notify_ipc_operation();
		};


		destroy_pfds();
	}
}

int
server(char *cfg_path) {
	log_set_times(true);

	// only one instance
	pid_file_create();

	// don't log anything until cfg log level is known
	log_capture_start();
	log_suppress_start();

	log_info("way-displays version %s", VERSION);

	// all cfg paths
	cfg_file_paths_init(cfg_path);

	// maybe default, never exits
	cfg_init(cfg_path);
	free(cfg_path);

	// play back captured logs from cfg parse
	log_set_threshold(cfg->log_threshold, false);
	log_suppress_stop();
	log_capture_stop();
	log_capture_playback(NULL);
	log_capture_clear();

	// discover the lid state immediately
	lid_init();
	lid_update();

	// discover the output manager; it will call back
	displ_init();

	// only stops when signalled or display goes away
	int sig = loop();

	// release what resources we can
	heads_destroy();
	lid_destroy();
	cfg_file_paths_destroy();
	cfg_destroy();
	displ_destroy();

	return sig;
}

