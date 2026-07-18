#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "client.h"

#include "enum.h"
#include "info/print.h"
#include "ipc.h"
#include "log.h"
#include "process.h"
#include "pset.h"

static int handle_responses(const struct IpcRequest *ipc_request) {
	int rc = EXIT_SUCCESS;

	const struct Pset *responses = NULL;
	const struct IpcResponse *response = NULL;
	bool done = false;

	while (!done) {
		char *yaml;
		responses = ipc_receive_responses(ipc_request->socket_client, &yaml);

		if (responses) {
			for (const struct PsetIt *it = pset_it(responses); it; it = pset_it_next(it)) {
				if (!(response = it->val)) {
					continue;
				}
				rc = response->status.rc;
				done = response->status.done;

				if (ipc_request->yaml) {
					if (yaml && (rc < IPC_ERROR)) {
						// yaml
						fprintf(stdout, "%s\n", yaml);
					} else {
						// human errors
						log_cap_lines_playback(response->log_cap_lines);
					}
				} else {
					// human
					log_cap_lines_playback(response->log_cap_lines);
				}
			}
			pset_free_vals(responses);
		} else {
			rc = IPC_BAD_RESPONSE;
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

	char pid_path[PATH_MAX];
	pid_path_generate(pid_path);

	if (pid_active_server(pid_path) == 0) {
		log_fatal("way-displays not running, check $XDG_VTNR");
		rc = EXIT_FAILURE;
		goto end;
	}

	if (!ipc_request->yaml) {
		log_debug(NULL);
		log_debug("Client sending request: %s", ipc_command_name(ipc_request->command));
		print_cfg(DEBUG, ipc_request->cfg, ipc_request->command == CFG_DEL);
	}

	ipc_send_request(ipc_request);

	if (ipc_request->socket_client == -1) {
		rc = EXIT_FAILURE;
		goto end;
	}

	rc = handle_responses(ipc_request);

	close(ipc_request->socket_client);

end:
	ipc_request_free(ipc_request);

	return rc;
}

