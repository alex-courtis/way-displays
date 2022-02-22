// IWYU pragma: no_include <bits/getopt_core.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "client.h"

#include "ipc.h"
#include "log.h"
#include "process.h"


int client(struct IpcRequest *ipc_request) {
	if (!ipc_request) {
		return EXIT_FAILURE;
	}

	log_set_times(false);

	int rc = EXIT_SUCCESS;

	if (pid_active_server() == 0) {
		log_error("way-displays not running");
		rc = EXIT_FAILURE;
		goto end;
	}

	int fd = ipc_request_send(ipc_request);
	if (fd == -1) {
		rc = EXIT_FAILURE;
		goto end;
	}

	struct IpcResponse *ipc_response;
	bool done = false;
	while (!done) {
		ipc_response = ipc_response_receive(fd);
		rc = ipc_response->rc;
		done = ipc_response->done;
		free_ipc_response(ipc_response);
	}

	close(fd);

end:
	free_ipc_request(ipc_request);

	return rc;
}

