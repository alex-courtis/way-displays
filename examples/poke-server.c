#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <unistd.h>

#include "process.h"
#include "sockets.h"

/*
 * Check environment and locate the server socket and pid
 */
int
main(int argc, char **argv) {

	if (!getenv("WAYLAND_DISPLAY")) {
		fprintf(stderr, "environment variable $WAYLAND_DISPLAY missing\n");
		exit(1);
	}
	fprintf(stdout, "WAYLAND_DISPLAY=%s\n", getenv("WAYLAND_DISPLAY") ? getenv("WAYLAND_DISPLAY") : "");
	fprintf(stdout, "XDG_VTNR=%s\n", getenv("XDG_VTNR") ? getenv("XDG_VTNR") : "");

	char *path = pid_path();
	fprintf(stdout, "Server PID file %s\n", path);
	free(path);

	pid_t pid = pid_active_server();
	if (pid == 0) {
		fprintf(stderr, "way-displays not running\n");
		exit(1);
	}
	fprintf(stdout, "Server PID %d\n", pid);

	struct sockaddr_un addr;
	socket_path(&addr);
	fprintf(stdout, "Server socket %s\n", addr.sun_path);

	return(0);
}

