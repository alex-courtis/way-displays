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
#include "slist.h"

// handle one response and end
int handle_response(const struct IpcRequest *ipc_request) {
	char *yaml =ipc_receive_raw(ipc_request->socket_client);

	fprintf(stdout, "%s\n", yaml);

	free(yaml);

	return EXIT_SUCCESS;
}

// handle many responses over many socket operations
int handle_responses(const struct IpcRequest *ipc_request) {
	int rc = EXIT_SUCCESS;

	struct SList *responses = NULL;
	struct IpcResponse *response = NULL;
	bool done = false;

	while (!done) {
		char *yaml;
		responses = ipc_receive_responses(ipc_request->socket_client, &yaml);

		if (responses) {
			for (struct SList *i = responses; i; i = i->nex) {
				if (!(response = i->val)) {
					continue;
				}
				rc = response->status.rc;
				done = response->status.done;

				if (ipc_request->yaml) {
					if (yaml && (rc == IPC_RC_SUCCESS || rc == IPC_RC_WARN)) {
						// yaml
						fprintf(stdout, "%s\n", yaml);
					} else {
						// human errors
						log_capture_playback(response->log_cap_lines);
					}
				} else {
					// human
					log_capture_playback(response->log_cap_lines);
				}
			}
			slist_free_vals(&responses, ipc_response_free);
		} else {
			rc = IPC_RC_BAD_RESPONSE;
			done = true;
		}

		free(yaml);
	}

	return rc;
}

int client(struct IpcRequest *ipc_request) {
	if (!ipc_request) {
		return EXIT_FAILURE;
	}

	int rc = EXIT_SUCCESS;

	if (pid_active_server() == 0) {
		log_error("way-displays not running, check $XDG_VTNR");
		rc = EXIT_FAILURE;
		goto end;
	}

	if (!ipc_request->yaml) {
		log_info("\nClient sending request: %s", ipc_command_friendly(ipc_request->command));
		print_cfg(INFO, ipc_request->cfg, ipc_request->command == CFG_DEL);
	}

	ipc_send_request(ipc_request);

	if (ipc_request->socket_client == -1) {
		rc = EXIT_FAILURE;
		goto end;
	}

	if (ipc_request->command == GET) {
		rc = handle_response(ipc_request);
	} else {
		rc = handle_responses(ipc_request);
	}

	close(ipc_request->socket_client);

end:
	ipc_request_free(ipc_request);

	return rc;
}

