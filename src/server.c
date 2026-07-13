#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include "wl_wrappers.h"

#include "server.h"

#include "act.h"
#include "cfg.h"
#include "cfg/file.h"
#include "convert.h"
#include "displ.h"
#include "fds.h"
#include "head.h"
#include "info/print.h"
#include "ipc.h"
#include "lid.h"
#include "log.h"
#include "process.h"
#include "pslist.h"

// operation in progress
struct IpcOperation *ipc_operation = NULL;

// received a request whilst another is in progress
static void handle_ipc_in_progress(int server_socket) {
	struct IpcRequest *request = ipc_receive_request(server_socket);
	if (!request) {
		log_error(NULL);
		log_error("Failed to read IPC request");
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

static void notify_ipc_operation(void) {
	if (!ipc_operation) {
		return;
	}

	ipc_send_operation(ipc_operation);

	if (ipc_operation->done) {
		close(ipc_operation->socket_client);

		log_cap_lines_stop(&ipc_operation->log_cap_lines);
		ipc_operation_free(ipc_operation);
		ipc_operation = NULL;
	}
}

static void receive_ipc_request(int server_socket) {
	if (ipc_operation) {
		handle_ipc_in_progress(server_socket);
		return;
	}

	ipc_operation = (struct IpcOperation*)calloc(1, sizeof(struct IpcOperation));
	log_cap_lines_start(&ipc_operation->log_cap_lines);

	struct IpcRequest *ipc_request = ipc_receive_request(server_socket);
	if (!ipc_request) {
		log_error(NULL);
		log_error("Failed to read IPC request");
		log_cap_lines_stop(&ipc_operation->log_cap_lines);
		ipc_operation_free(ipc_operation);
		ipc_operation = NULL;
		return;
	}

	ipc_operation->request = ipc_request;
	ipc_operation->socket_client = ipc_request->socket_client;
	ipc_operation->done = true;
	ipc_operation->send_state = true;

	if (ipc_request->bad) {
		ipc_operation->rc = IPC_RC_BAD_REQUEST;
		ipc_operation->send_state = false;
		goto send;
	}

	log_debug(NULL);
	log_debug("Server received request: %s", ipc_command_friendly(ipc_request->command));
	if (ipc_request->cfg) {
		print_cfg(DEBUG, ipc_request->cfg, ipc_request->command == CFG_DEL);
	}

	// handle extra toggles
	if (ipc_request->command == CFG_TOGGLE) {
		for (struct Pslist *i = g_heads; i; i = i->nex) {
			head_apply_toggles(i->val, ipc_request->cfg);
		}
	}

	switch (ipc_request->command) {
		case CFG_DEL:
		case CFG_SET:
		case CFG_TOGGLE:
			{
				struct Cfg *cfg_merged = cfg_merge(g_cfg, ipc_request->cfg, ipc_request->command);
				if (cfg_merged) {
					// ongoing
					ipc_operation->done = false;
					cfg_free(g_cfg);
					g_cfg = cfg_merged;
					log_info(NULL);
					log_info("New configuration:");
					print_cfg(INFO, g_cfg, false);
				} else {
					// complete
					log_info(NULL);
					log_info("No config changes to make.");
				}
				break;
			}
		case CFG_WRITE:
			{
				// complete
				g_cfg_file_write();
				break;
			}
		case LIST:
			{
				// complete
				print_list(INFO, g_heads);
				break;
			}
		case REAPPLY:
			{
				// ongoing
				ipc_operation->done = false;
				heads_reapply(g_heads);
				break;
			}
		case GET:
		default:
			{
				// complete
				log_info(NULL);
				log_info("Active configuration:");
				print_cfg(INFO, g_cfg, false);
				print_cfg_commands(INFO, g_cfg);
				print_heads(INFO, NONE, g_heads);
				break;
			}
	}

send:
	notify_ipc_operation();
}

// see Wayland Protocol docs Appendix B wl_display_prepare_read_queue
static int loop(void) {

	for (;;) {
		pfds_init();

		// prepare for reading wayland events
		while (_wl_display_prepare_read(g_displ->display, FL) != 0) {
			_wl_display_dispatch_pending__prepare_read(g_displ->display, FL);
		}
		_wl_display_flush(g_displ->display, FL);

		// poll for all events
		if (poll(pfds, npfds, -1) < 0) {
			log_fatal(NULL);
			log_fatal_errno("poll failed, exiting");
			wd_exit_message(EXIT_FAILURE);
			return EXIT_FAILURE;
		}

		// always read and dispatch wayland events; stop the file descriptor from getting stale
		if (_wl_display_read_events(g_displ->display, FL) == -1)
			return EXIT_SUCCESS;
		_wl_display_dispatch_pending__read_events(g_displ->display, FL);

		if (!g_displ->zwlr_output_manager) {
			log_info(NULL);
			log_info("Display's output manager has departed, exiting");
			wd_exit(EXIT_SUCCESS);
			return EXIT_SUCCESS;
		}

		// subscribed signals are mostly a clean exit
		if (pfd_signal && pfd_signal->revents & pfd_signal->events) {
			struct signalfd_siginfo fdsi;
			if (read(fd_signal, &fdsi, sizeof(fdsi)) == sizeof(fdsi)) {
				log_debug("Received signal %d: %s", fdsi.ssi_signo, strsignal(fdsi.ssi_signo));
				if (fdsi.ssi_signo != SIGPIPE) {
					log_info(NULL);
					log_info("Received signal %d: %s, exiting", fdsi.ssi_signo, strsignal(fdsi.ssi_signo));
					return fdsi.ssi_signo;
				}
			}
		}

		// cfg directory change
		if (pfd_cfg_dir && pfd_cfg_dir->revents & pfd_cfg_dir->events) {
			if (fd_cfg_dir_modified(g_cfg_file->file_name)) {
				if (g_cfg_file->modified) {
					g_cfg_file->modified = false;
				} else {
					g_cfg_file_reload();
				}
			}
		}

		// libinput lid event
		if (pfd_lid && pfd_lid->revents & pfd_lid->events) {
			g_lid_update();
		}

		// ipc client message
		if (pfd_ipc && (pfd_ipc->revents & pfd_ipc->events)) {
			receive_ipc_request(fd_socket_server);
		}

		// maybe make some changes
		act();

		// inform the client
		if (ipc_operation) {
			ipc_operation->done = g_displ->state == IDLE;
			notify_ipc_operation();
		};

		pfds_destroy();
	}
}

static void setup_signal_handlers(void) {
	struct sigaction sa;

	// don't transform child processes into zombies and don't handle SIGCHLD.
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT | SA_RESTART;
	sa.sa_handler = SIG_DFL;
	sigaction(SIGCHLD, &sa, NULL);
}

int
server(char *cfg_path) {
	// exits when another instance running
	pid_file_create();

	log_set_prefix(true);

	setup_signal_handlers();

	// don't log anything until cfg log level is known
	struct Pslist *log_cap_lines = NULL;
	log_cap_lines_start(&log_cap_lines);
	log_suppress_start();

	log_info("way-displays version %s %s", VERSION, COMMIT);

	// maybe default, never exits
	g_cfg_file_init_read(cfg_path);
	free(cfg_path);

	// play back captured logs from cfg parse
	log_set_threshold(g_cfg->log_threshold, false);
	log_suppress_stop();
	log_cap_lines_stop(&log_cap_lines);
	log_cap_lines_playback(log_cap_lines);
	log_cap_lines_free(&log_cap_lines);

	// discover the lid state immediately
	g_lid_init();
	g_lid_update();

	// discover the output manager; it will call back
	g_displ_init();

	// only stops when signalled or display goes away
	int sig = loop();

	// release what resources we can
	g_heads_destroy();
	g_lid_destroy();
	g_cfg_file_destroy();
	g_cfg_destroy();
	g_displ_destroy();

	return sig;
}
