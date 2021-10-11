#include <poll.h>
#include <signal.h>
#include <sys/inotify.h>
#include <sys/signalfd.h>

#include "fds.h"

#include <stdio.h>

int fd_signal = 0;

int npfds = 0;
struct pollfd *pfds = NULL;

struct pollfd *pfd_signal = NULL;
struct pollfd *pfd_wayland = NULL;
struct pollfd *pfd_lid = NULL;

void create_fd_signal() {
	if (!fd_signal) {
		sigset_t mask;
		sigemptyset(&mask);
		sigaddset(&mask, SIGINT);
		sigaddset(&mask, SIGQUIT);
		sigaddset(&mask, SIGTERM);
		sigprocmask(SIG_BLOCK, &mask, NULL);
		fd_signal = signalfd(-1, &mask, 0);
	}
}

void create_pfds(struct Displ *displ) {
	if (!displ)
		return;

	create_fd_signal();

	npfds = 2;
	if (displ->lid)
		npfds++;

	pfds = calloc(npfds, sizeof(struct pollfd));

	int i = 0;

	pfd_signal = &pfds[i++];
	pfd_signal->fd = fd_signal;
	pfd_signal->events = POLLIN;

	pfd_wayland = &pfds[i++];
	pfd_wayland->fd = wl_display_get_fd(displ->display);
	pfd_wayland->events = POLLIN;

	if (displ->lid) {
		pfd_lid = &pfds[i++];
		pfd_lid->fd = displ->lid->libinput_fd;
		pfd_lid->events = POLLIN;
	}
}

void destroy_pfds() {
	npfds = 0;

	pfd_signal = NULL;
	pfd_wayland = NULL;
	pfd_lid = NULL;

	free(pfds);
}

