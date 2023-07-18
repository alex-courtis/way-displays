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

	char *yaml = ipc_receive_raw(socket_client);
	while (yaml) {
		fprintf(stdout, "%s", yaml);
		free(yaml);
		yaml = ipc_receive_raw(socket_client);
	}

	return rc;
}

int handle_human(int socket_client) {
	int rc = EXIT_SUCCESS;

	struct IpcResponseStatus *response_status = NULL;
	bool done = false;

	while (!done) {
		response_status = ipc_receive_responses_log(socket_client);
		if (response_status) {
			rc = response_status->rc;
			done = response_status->done;
			ipc_response_status_free(response_status);
			response_status = NULL;
		} else {
			rc = IPC_RC_BAD_RESPONSE;
			done = true;
		}
	}

	if (response_status) {
		ipc_response_status_free(response_status);
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

	log_info("\nClient sending request: %s", ipc_command_friendly(ipc_request->command));
	print_cfg(INFO, ipc_request->cfg, ipc_request->command == CFG_DEL);

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

