#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include <unistd.h>
#include <wayland-client-core.h>

#include "fds.h"

#include "cfg.h"
#include "displ.h"
#include "lid.h"
#include "log.h"
#include "global.h"
#include "process.h"
#include "sockets.h"

#define PFDS_SIZE 5

int fd_signal = -1;
int fd_socket_server = -1;
int fd_cfg_dir = -1;
int wd_cfg_dir = -1;
bool fds_created = false;

nfds_t npfds = 0;
struct pollfd pfds[PFDS_SIZE];

struct pollfd *pfd_signal = NULL;
struct pollfd *pfd_ipc = NULL;
struct pollfd *pfd_wayland = NULL;
struct pollfd *pfd_lid = NULL;
struct pollfd *pfd_cfg_dir = NULL;

int create_fd_signal(void) {
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGPIPE);
	sigprocmask(SIG_BLOCK, &mask, NULL);
	return signalfd(-1, &mask, 0);
}

void fd_wd_cfg_dir_create(void) {
	if (!cfg->dir_path)
		return;

	fd_cfg_dir = inotify_init1(IN_NONBLOCK);
	if ((wd_cfg_dir = inotify_add_watch(fd_cfg_dir, cfg->dir_path, IN_CLOSE_WRITE)) == -1) {
		close(fd_cfg_dir);
		fd_cfg_dir = -1;
		log_error_errno("\nunable to create config directory watch for %s, exiting", cfg->dir_path);
		wd_exit_message(EXIT_FAILURE);
		return;
	}
}

void fd_wd_cfg_dir_destroy(void) {
	if (fd_cfg_dir == -1 || wd_cfg_dir == -1) {
		fd_cfg_dir = -1;
		wd_cfg_dir = -1;
		return;
	}

	if (inotify_rm_watch(fd_cfg_dir, wd_cfg_dir) == -1) {
		log_error_errno("\nunable to remove config directory watch");
	}

	if (close(fd_cfg_dir) == -1) {
		log_error_errno("\nunable to close config directory watch");
	}

	fd_cfg_dir = -1;
	wd_cfg_dir = -1;
}

void create_fds(void) {
	fd_signal = create_fd_signal();
	fd_socket_server = create_socket_server();
	fd_wd_cfg_dir_create();

	fds_created = true;
}

void pfds_init(void) {
	if (!fds_created)
		create_fds();

	// wayland and signal are always present, others are optional
	npfds = 2;
	if (lid)
		npfds++;
	if (fd_socket_server != -1)
		npfds++;
	if (fd_cfg_dir != -1)
		npfds++;

	int i = 0;

	pfd_signal = &pfds[i++];
	pfd_signal->fd = fd_signal;
	pfd_signal->events = POLLIN;

	pfd_wayland = &pfds[i++];
	pfd_wayland->fd = wl_display_get_fd(displ->display);
	pfd_wayland->events = POLLIN;

	if (fd_socket_server != -1) {
		pfd_ipc = &pfds[i++];
		pfd_ipc->fd = fd_socket_server;
		pfd_ipc->events = POLLIN;
	}

	if (lid) {
		pfd_lid = &pfds[i++];
		pfd_lid->fd = lid->libinput_fd;
		pfd_lid->events = POLLIN;
	}

	if (fd_cfg_dir != -1) {
		pfd_cfg_dir = &pfds[i++];
		pfd_cfg_dir->fd = fd_cfg_dir;
		pfd_cfg_dir->events = POLLIN;
	}
}

void pfds_destroy(void) {
	npfds = 0;

	pfd_signal = NULL;
	pfd_wayland = NULL;
	pfd_lid = NULL;
	pfd_ipc = NULL;
	pfd_cfg_dir = NULL;

	for (size_t i = 0; i < PFDS_SIZE; i++) {
		pfds[i].fd = 0;
		pfds[i].events = 0;
		pfds[i].revents = 0;
	}
}

// see man 7 inotify
bool fd_cfg_dir_modified(char *file_name) {
	if (!file_name) {
		return false;
	}

	char buf[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
	const struct inotify_event *event;
	ssize_t len;

	while ((len = read(fd_cfg_dir, buf, sizeof(buf))) > 0) {
		for (char *ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) {
			event = (const struct inotify_event *) ptr;
			if (event->mask & IN_CLOSE_WRITE && event->len && strcmp(file_name, event->name) == 0) {
				return true;
			}
		}
	}

	return false;
}

