#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "client.h"

#include "convert.h"
#include "info.h"
#include "ipc.h"
#include "log.h"
#include "process.h"

int handle_raw(int socket_client) {
	int rc = EXIT_SUCCESS;

	char *yaml = ipc_receive_raw_client(socket_client);
	while (yaml) {
		fprintf(stdout, "%s", yaml);
		free(yaml);
		yaml = ipc_receive_raw_client(socket_client);
	}

	return rc;
}

int handle_human(int socket_client) {
	int rc = EXIT_SUCCESS;

	struct IpcResponse *response;
	bool done = false;

	while (!done) {
		response = ipc_receive_response_client(socket_client);
		if (response) {
			rc = response->rc;
			done = response->done;
		} else {
			rc = IPC_RC_BAD_RESPONSE;
			done = true;
		}
	}

	if (response) {
		ipc_response_free(response);
	}

	return rc;
}

int client(struct IpcRequest *ipc_request) {
	if (!ipc_request) {
		return EXIT_FAILURE;
	}

	if (ipc_request->raw) {
		log_set_threshold(ERROR, true);
	}

	log_set_times(false);

	int rc = EXIT_SUCCESS;

	if (pid_active_server() == 0) {
		log_error("way-displays not running, check $XDG_VTNR");
		rc = EXIT_FAILURE;
		goto end;
	}

	log_info("\nClient sending request: %s", ipc_request_op_friendly(ipc_request->op));
	print_cfg(INFO, ipc_request->cfg, ipc_request->op == CFG_DEL);

	ipc_send_request(ipc_request);

	if (ipc_request->socket_client == -1) {
		rc = EXIT_FAILURE;
		goto end;
	}

	if (ipc_request->raw) {
		rc = handle_raw(ipc_request->socket_client);
	} else {
		rc = handle_human(ipc_request->socket_client);
	}

	close(ipc_request->socket_client);

end:
	ipc_request_free(ipc_request);

	return rc;
}

