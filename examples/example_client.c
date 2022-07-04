#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "convert.h"
#include "ipc.h"
#include "process.h"
#include "sockets.h"

void execute(enum IpcRequestCommand command, char *request) {
	int fd;

	if ((fd = create_fd_ipc_client()) == -1) {
		exit(1);
	}

	log_debug("========%s request=================\n%s\n----------------------------------------", ipc_request_command_name(command), request);
	if (socket_write(fd, request, strlen(request)) == -1) {
		exit(1);
	}

	for (;;) {
		char *response = NULL;
		if (!(response = socket_read(fd))) { // yup, that's a memory leak
			exit(1);
		}
		log_debug("========%s response================\n%s\n----------------------------------------", ipc_request_command_name(command), response);
		if (strstr(response, "DONE: TRUE")) {
			break;
		}
	}

	close(fd);
}

void cfg_get(void) {
	char *request = "\
CFG_GET:\n\
";

	execute(CFG_GET, request);
}

void cfg_write(void) {
	char *request = "\
CFG_WRITE:\n\
";

	execute(CFG_WRITE, request);
}

void cfg_set(void) {
	char *request = "\
CFG_SET:\n\
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
CFG_DEL:\n\
  SCALE:\n\
    - NAME_DESC: DEF 456\n\
      SCALE: 1\n\
  MODE:\n\
    - NAME_DESC: GHI 789\n\
  DISABLED:\n\
    - PQR 678\n\
    - STU 901\n\
";

	execute(CFG_DEL, request);
}

void usage(void) {
	fprintf(stderr, "Usage: example_client <CFG_GET | CFG_WRITE | CFG_SET | CFG_DEL>\n");
	exit(1);
}

int
main(int argc, char **argv) {
	log_set_threshold(DEBUG, true);

	if (argc != 2) {
		usage();
	}

	void (*fn)(void);
	if (strcmp(argv[1], ipc_request_command_name(CFG_GET)) == 0) {
		fn = cfg_get;
	} else if (strcmp(argv[1], ipc_request_command_name(CFG_WRITE)) == 0) {
		fn = cfg_write;
	} else if (strcmp(argv[1], ipc_request_command_name(CFG_SET)) == 0) {
		fn = cfg_set;
	} else if (strcmp(argv[1], ipc_request_command_name(CFG_DEL)) == 0) {
		fn = cfg_del;
	} else {
		usage();
	}

	if (!getenv("XDG_VTNR")) {
		log_error("environment variable $XDG_VTNR missing");
		exit(1);
	}
	log_debug("XDG_VTNR=%s", getenv("XDG_VTNR"));

	if (pid_active_server() == 0) {
		log_error("way-displays not running");
		exit(1);
	}

	fn();

	return(0);
}

