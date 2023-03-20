#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "cli.h"
#include "client.h"
#include "log.h"
#include "server.h"

int
main(int argc, char **argv) {
	setlinebuf(stdout);

	if (!getenv("WAYLAND_DISPLAY")) {
		log_error("environment variable $WAYLAND_DISPLAY missing");
		return EXIT_FAILURE;
	}

	// consumer frees
	struct IpcRequest *ipc_request = NULL;
	char *cfg_path = NULL;

	parse_args(argc, argv, &ipc_request, &cfg_path);

	if (ipc_request) {
		return client(ipc_request);
	} else {
		return server(cfg_path);
	}
}

