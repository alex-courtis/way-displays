#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "cfg.h"
#include "convert.h"
#include "head.h"
#include "ipc.h"
#include "log.h"
#include "sockets.h"

/*
 * Execute a CFG_SET scaling off and unpack the responses
 */
int
main(int argc, char **argv) {
	char *yaml;
	struct SList *responses = NULL;
	struct IpcResponse *response = NULL;
	struct Head *head = NULL;

	// request CFG_SET
	struct IpcRequest *request = calloc(1, sizeof(struct IpcRequest));
	request->command = CFG_SET;

	// turn scaling ON
	request->cfg = calloc(1, sizeof(struct Cfg));
	request->cfg->scaling = ON;

	// send the request
	ipc_send_request(request);
	if (request->socket_client == -1) {
		exit(IPC_RC_BAD_REQUEST);
	}

	struct IpcResponseStatus status = { 0 };
	status.done = false;
	while (!status.done) {

		// listen to the socket
		responses = ipc_receive_responses(request->socket_client, &yaml);
		if (!responses) {
			status.done = true;
			status.rc = IPC_RC_BAD_RESPONSE;
		}
		log_info("================================");

		// parse one to many responses
		for (struct SList *i = responses; i; i = i->nex) {
			response = i->val;
			log_info("--------------------------------");

			// status informs whether there are more messages
			status = response->status;

			// inspect config
			log_info("scaling is %s", on_off_name(response->cfg->scaling));

			// inspect head state
			for (struct SList *j = response->heads; j; j = j->nex) {
				head = j->val;
				float scale_current = wl_fixed_to_double(head->current.scale);
				float scale_desired = wl_fixed_to_double(head->desired.scale);

				log_info("%s", head->description);
				if (scale_current == scale_desired) {
					log_info("  scale %g", scale_current);
				} else {
					log_info("  scale %g -> %g", scale_current, scale_desired);
				}
			}
		}

		slist_free_vals(&responses, ipc_response_free);
		free(yaml);
	}

	ipc_request_free(request);

	return status.rc;
}

