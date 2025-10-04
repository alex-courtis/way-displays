#include <stdio.h>
#include <stdlib.h>

#include "cli.h"
#include "client.h"
#include "log.h"
#include "server.h"

int
main(int argc, char **argv) {
	setlinebuf(stdout);

	if (getenv("HYPRLAND_INSTANCE_SIGNATURE")) {
		log_warn("Hpyrland already provides all the features of `way-displays`. It may function, however it is explicitly not supported and you will likely experience problems. Please do not raise issues.");
	}

	// consumer frees
	struct IpcRequest *ipc_request = NULL;
	char *cfg_path = NULL;

	parse_args(argc, argv, &ipc_request, &cfg_path);

	if (!getenv("WAYLAND_DISPLAY")) {
		log_fatal("environment variable $WAYLAND_DISPLAY missing");
		return EXIT_FAILURE;
	}

	if (ipc_request) {
		return client(ipc_request);
	} else {
		return server(cfg_path);
	}
}

