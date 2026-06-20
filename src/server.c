#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
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
#include "slist.h"
#include "yaml/unmarshal.h"
#include "yaml/unmarshal-types.h"

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
		for (struct SList *i = g_heads; i; i = i->nex) {
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
				cfg_file_write();
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

void load_cfg(void) {
	struct Cfg *cfg_resolved = cfg_init();

	bool resolved = cfg_resolve_file_path(cfg_resolved);

	if (resolved) {
		log_info(NULL);
		log_info("Found configuration file: %s", cfg_resolved->file_path);

		g_cfg = yaml_unmarshal_file(cfg_resolved->file_path, yaml_root_to_cfg);

		if (!g_cfg) {
			log_info(NULL);
			log_info("Using default configuration:");
			g_cfg = cfg_init();
		}
	} else {
		log_info(NULL);
		log_info("No configuration file found, using defaults:");
		g_cfg = cfg_init();
	}

	cfg_apply_defaults(g_cfg);
	cfg_copy_file_path(g_cfg, cfg_resolved);

	validate_fix(g_cfg);
	log_info(NULL);
	log_info("Active configuration:");
	print_cfg(INFO, g_cfg, false);
	validate_warn(g_cfg);

	cfg_free(cfg_resolved);
}

void reload_cfg(void) {
	if (!g_cfg || !g_cfg->file_path)
		return;

	log_info(NULL);
	log_info("Reloading configuration file: %s", g_cfg->file_path);

	struct Cfg *cfg_loaded = yaml_unmarshal_file(g_cfg->file_path, yaml_root_to_cfg);

	if (cfg_loaded) {
		cfg_apply_defaults(cfg_loaded);
		cfg_copy_file_path(cfg_loaded, g_cfg);

		cfg_free(g_cfg);
		g_cfg = cfg_loaded;

		log_set_threshold(g_cfg->log_threshold, false);
		validate_fix(g_cfg);
		log_info(NULL);
		log_info("New configuration:");
		print_cfg(INFO, g_cfg, false);
		validate_warn(g_cfg);

	} else {
		log_info(NULL);
		log_info("Configuration unchanged:");
		print_cfg(INFO, g_cfg, false);
	}
}

// see Wayland Protocol docs Appendix B wl_display_prepare_read_queue
static int loop(void) {

	for (;;) {
		log_debug("LOOP START");

		log_debug("LOOP pfds_init");
		pfds_init();

		// prepare for reading wayland events
		log_debug("LOOP _wl_display_prepare_read");
		while (_wl_display_prepare_read(g_displ->display, FL) != 0) {
			log_debug("LOOP _wl_display_dispatch_pending__prepare_read");
			_wl_display_dispatch_pending__prepare_read(g_displ->display, FL);
		}

		log_debug("LOOP _wl_display_flush");
		_wl_display_flush(g_displ->display, FL);


		// poll for all events
		log_debug("LOOP poll");
		if (poll(pfds, npfds, -1) < 0) {
			log_fatal(NULL);
			log_fatal_errno("poll failed, exiting");
			wd_exit_message(EXIT_FAILURE);
			return EXIT_FAILURE;
		}


		// always read and dispatch wayland events; stop the file descriptor from getting stale
		log_debug("LOOP _wl_display_read_events");
		if (_wl_display_read_events(g_displ->display, FL) == -1)
			return EXIT_SUCCESS;

		log_debug("LOOP _wl_display_dispatch_pending__read_events");
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
				log_debug("LOOP signal %d: %s", fdsi.ssi_signo, strsignal(fdsi.ssi_signo));
				if (fdsi.ssi_signo != SIGPIPE) {
					log_info(NULL);
					log_info("Received signal %d: %s, exiting", fdsi.ssi_signo, strsignal(fdsi.ssi_signo));
					return fdsi.ssi_signo;
				}
			}
		}


		// cfg directory change
		if (pfd_cfg_dir && pfd_cfg_dir->revents & pfd_cfg_dir->events) {
			if (fd_cfg_dir_modified(g_cfg->file_name)) {
				if (g_cfg->updated) {
					g_cfg->updated = false;
				} else {
					log_debug("LOOP cfg_file_reload");
					reload_cfg();
				}
			}
		}


		// libinput lid event
		if (pfd_lid && pfd_lid->revents & pfd_lid->events) {
			log_debug("LOOP lid_update");
			lid_update();
		}


		// ipc client message
		if (pfd_ipc && (pfd_ipc->revents & pfd_ipc->events)) {
			log_debug("LOOP receive_ipc_request");
			receive_ipc_request(fd_socket_server);
		}


		// maybe make some changes
		log_debug("LOOP layout");
		layout();


		// inform the client
		if (ipc_operation) {
			ipc_operation->done = g_displ->state == IDLE;
			log_debug("LOOP notify_ipc_operation");
			notify_ipc_operation();
		};


		log_debug("LOOP pfds_destroy");
		pfds_destroy();

		log_debug("LOOP END");
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
	struct SList *log_cap_lines = NULL;
	log_cap_lines_start(&log_cap_lines);
	log_suppress_start();

	log_info("way-displays version %s", VERSION);

	// all cfg paths
	cfg_file_paths_init(cfg_path);

	// maybe default, never exits
	load_cfg();
	free(cfg_path);

	// play back captured logs from cfg parse
	log_set_threshold(g_cfg->log_threshold, false);
	log_suppress_stop();
	log_cap_lines_stop(&log_cap_lines);
	log_cap_lines_playback(log_cap_lines);
	log_cap_lines_free(&log_cap_lines);

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

