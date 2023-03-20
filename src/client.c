#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "client.h"

#include "convert.h"
#include "info.h"
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
		log_error("way-displays not running, check $XDG_VTNR");
		rc = EXIT_FAILURE;
		goto end;
	}

	log_info("\nClient sending request: %s", ipc_request_op_friendly(ipc_request->op));
	print_cfg(INFO, ipc_request->cfg, ipc_request->op == CFG_DEL);

	int fd = ipc_request_send(ipc_request);
	if (fd == -1) {
		rc = EXIT_FAILURE;
		goto end;
	}

	struct IpcResponse *ipc_response;
	bool done = false;
	while (!done) {
		ipc_response = ipc_response_receive(fd);
		if (ipc_response) {
			rc = ipc_response->rc;
			done = ipc_response->done;
		} else {
			rc = IPC_RC_BAD_RESPONSE;
			done = true;
		}
	}

	if (ipc_response) {
		ipc_response_free(ipc_response);
	}

	close(fd);

end:
	ipc_request_free(ipc_request);

	return rc;
}

