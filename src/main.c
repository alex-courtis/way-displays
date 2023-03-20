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
	bool yaml = false;

	parse_args(argc, argv, &ipc_request, &cfg_path, &yaml);

	if (ipc_request) {
		return client(ipc_request, yaml);
	} else {
		return server(cfg_path);
	}
}

