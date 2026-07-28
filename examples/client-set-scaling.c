#include <stdbool.h>
#include <stdlib.h>
#include <wayland-util.h>

#include "cfg/cfg.h"
#include "enum.h"
#include "head.h"
#include "ipc.h"
#include "log.h"
#include "plist.h"
#include "pset.h"

/*
 * Execute a CFG_SET scaling off and unpack the responses
 */
int
main(int argc, char **argv) {
	char *yaml;
	const struct Plist *responses = NULL;
	const struct IpcResponse *response = NULL;

	// request CFG_SET
	struct IpcRequest *request = ipc_request_init(CFG_SET);

	// turn scaling OFF
	request->cfg = cfg_init();
	request->cfg->scaling = OFF;

	// send the request
	ipc_send_request(request);
	if (request->socket_client == -1) {
		exit(IPC_BAD_REQUEST);
	}

	struct IpcResponseStatus status = { 0 };
	status.done = false;
	while (!status.done) {

		// listen to the socket
		responses = ipc_receive_responses(request->socket_client, &yaml);
		if (!responses) {
			status.done = true;
			status.rc = IPC_BAD_RESPONSE;
		}

		// parse one to many responses
		for (const struct PlistIt *rit = plist_it(responses); rit; rit = plist_it_next(rit)) {
			response = rit->val;
			log_info("--------------------------------");

			// status informs whether there are more messages
			status = response->status;

			// inspect config
			log_info("scaling is %s", on_off_name(response->cfg->scaling));

			// inspect head state
			for (const struct PsetIt *hit = pset_it(response->heads); hit; hit = pset_it_next(hit)) {
				const struct Head *head = hit->val;
				float scale_current = wl_fixed_to_double(head->cur.scale);
				float scale_desired = wl_fixed_to_double(head->des.scale);

				log_info("%s", head->description);
				if (scale_current == scale_desired) {
					log_info("  scale %g", scale_current);
				} else {
					log_info("  scale %g -> %g", scale_current, scale_desired);
				}
			}
		}

		plist_free_vals(responses);
		free(yaml);
	}

	log_info("--------------------------------");

	ipc_request_free(request);

	return status.rc;
}

