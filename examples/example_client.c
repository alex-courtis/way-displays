#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>

#include "convert.h"
#include "ipc.h"
#include "process.h"
#include "sockets.h"

void execute(enum IpcRequestOperation op, char *request) {
	int fd;

	if ((fd = create_fd_ipc_client()) == -1) {
		exit(1);
	}

	log_debug("========%s request=================\n%s\n----------------------------------------", ipc_request_op_name(op), request);
	if (socket_write(fd, request, strlen(request)) == -1) {
		exit(1);
	}

	for (;;) {
		char *response = NULL;
		if (!(response = socket_read(fd))) { // yup, that's a memory leak
			exit(1);
		}
		log_debug("========%s response================\n%s\n----------------------------------------", ipc_request_op_name(op), response);
		if (strstr(response, "DONE: TRUE")) {
			break;
		}
	}

	close(fd);
}

void get(void) {
	char *request = "\
OP: GET\n\
";

	execute(GET, request);
}

void cfg_write(void) {
	char *request = "\
OP: CFG_WRITE\n\
";

	execute(CFG_WRITE, request);
}

void cfg_set(void) {
	char *request = "\
OP: CFG_SET\n\
CFG: \n\
  ARRANGE: COL\n\
  ALIGN: RIGHT\n\
  ORDER:\n\
    - eDP-1\n\
    - ABC 123\n\
  AUTO_SCALE: ON\n\
  SCALE:\n\
    - NAME_DESC: DEF 456\n\
      SCALE: 3.5\n\
    - NAME_DESC: HDMI-1\n\
      SCALE: 1\n\
  MODE:\n\
    - NAME_DESC: GHI 789\n\
      WIDTH: 100\n\
      HEIGHT: 50\n\
      HZ: 88\n\
    - NAME_DESC: JKL 012\n\
      WIDTH: 200\n\
      HEIGHT: 100\n\
    - NAME_DESC: MNO 345\n\
      MAX: TRUE\n\
  DISABLED:\n\
    - PQR 678\n\
    - STU 901\n\
    - eDP-1\n\
  LAPTOP_DISPLAY_PREFIX: FFF\n\
";

	execute(CFG_SET, request);
}

void cfg_del(void) {
	char *request = "\
OP: CFG_DEL\n\
CFG:\n\
  SCALE:\n\
    - NAME_DESC: DEF 456\n\
      SCALE: 1\n\
  MODE:\n\
    - NAME_DESC: GHI 789\n\
  DISABLED:\n\
    - PQR 678\n\
    - STU 901\n\
    - eDP-1\n\
";

	execute(CFG_DEL, request);
}

void usage(void) {
	fprintf(stderr, "Usage: example_client <GET | CFG_WRITE | CFG_SET | CFG_DEL>\n");
	exit(1);
}

int
main(int argc, char **argv) {
	log_set_threshold(DEBUG, true);

	if (argc != 2) {
		usage();
	}

	void (*fn)(void);
	if (strcmp(argv[1], ipc_request_op_name(GET)) == 0) {
		fn = get;
	} else if (strcmp(argv[1], ipc_request_op_name(CFG_WRITE)) == 0) {
		fn = cfg_write;
	} else if (strcmp(argv[1], ipc_request_op_name(CFG_SET)) == 0) {
		fn = cfg_set;
	} else if (strcmp(argv[1], ipc_request_op_name(CFG_DEL)) == 0) {
		fn = cfg_del;
	} else {
		usage();
	}

	if (!getenv("WAYLAND_DISPLAY")) {
		log_error("environment variable $WAYLAND_DISPLAY missing");
		exit(1);
	}
	log_debug("WAYLAND_DISPLAY=%s", getenv("WAYLAND_DISPLAY") ? getenv("WAYLAND_DISPLAY") : "");
	log_debug("XDG_VTNR=%s", getenv("XDG_VTNR") ? getenv("XDG_VTNR") : "");

	char *path = pid_path();
	log_debug("Server PID file %s", path);
	free(path);

	pid_t pid = pid_active_server();
	if (pid == 0) {
		log_error("way-displays not running");
		exit(1);
	}
	log_debug("Server PID %d", pid);

	struct sockaddr_un addr;
	socket_path(&addr);
	log_debug("Server socket %s", addr.sun_path);

	fn();

	return(0);
}

