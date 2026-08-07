#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ipc.h"

#include "cfg/cfg.h"
#include "cfg/disabled.h"
#include "displ.h"
#include "enum.h"
#include "fn.h"
#include "head.h"
#include "lid.h"
#include "log.h"
#include "plist.h"
#include "ppmap.h"
#include "sockets.h"
#include "spmap.h"
#include "yaml/marshal-types.h"
#include "yaml/marshal.h"
#include "yaml/unmarshal-types.h"
#include "yaml/unmarshal.h"

struct IpcOperation *ipc_operation_init(void) {
	struct IpcOperation *operation = (struct IpcOperation*)calloc(1, sizeof(struct IpcOperation));
	operation->log_cap_lines = log_cap_line_plist_init();
	return operation;
}

struct IpcRequest *ipc_request_init(const enum IpcCommand command) {
	struct IpcRequest *request = calloc(1, sizeof(struct IpcRequest));
	request->command = command;
	return request;
}

struct IpcResponse *ipc_response_init(void) {
	struct IpcResponse *response = calloc(1, sizeof(struct IpcResponse));
	response->heads = head_plist_init();
	response->log_cap_lines = log_cap_line_plist_init();
	return response;
}

const struct Plist *ipc_response_plist_init(void) {
	return plist_init_with((struct PlistParams){ .free_val = (fn_free)ipc_response_free, });
}

void ipc_send_request(struct IpcRequest *request) {

	char *yaml = yaml_marshal(request, (fn_yaml_root_from_type)yaml_root_from_ipc_request, "ipc request");
	if (!yaml) {
		goto end;
	}

	// --yaml sets log level to warn hence this won't be printed
	log_debug("Client sending request YAML\n================================\n%s\n================================\n", yaml);

	if ((request->socket_client = create_socket_client()) == -1) {
		goto end;
	}

	if (socket_write(request->socket_client, yaml, strlen(yaml)) == -1) {
		request->socket_client = -1;
		goto end;
	}

end:
	if (yaml) {
		free(yaml);
	}
}

void ipc_operation_update_rc(struct IpcOperation *operation) {
	if (!operation)
		return;

	for (const struct PlistIt *it = plist_it(operation->log_cap_lines); it; it = plist_it_next(it)) {
		const struct LogCapLine *cap_line = (struct LogCapLine*)it->val;

		if (cap_line->threshold == WARNING && operation->rc < IPC_WARN)
			operation->rc = IPC_WARN;
		if (cap_line->threshold == ERROR && operation->rc < IPC_ERROR)
			operation->rc = IPC_ERROR;
	}
}

void ipc_request_filter(const struct IpcRequest *request) {
	if (!request || !request->cfg)
		return;

	// filter out and apply any disabled requests that affect conditionally disabled heads
	const struct SPmap *all_overridden = cfg_disabled_spmap_init();
	for (const struct PPmapIt *it = ppmap_it(g_displ->heads); it; it = ppmap_it_next(it)) {
		const struct SPmap *head_overridden = head_override_ipc_disableds((struct Head*)it->val, request);
		spmap_put_all(all_overridden, head_overridden);
		spmap_free(head_overridden);
	}
	spmap_remove_in_free(request->cfg->disableds, all_overridden);
	spmap_free(all_overridden);

	// filter out any disabled requests that are present as conditionals that don't affect current heads
	cfg_disabled_filter_conditional_clashes(request->cfg->disableds);
}

void ipc_send_operation(struct IpcOperation *operation) {
	ipc_operation_update_rc(operation);

	char *yaml = yaml_marshal(operation, (fn_yaml_root_from_type)yaml_root_from_ipc_operation, "ipc response");

	log_debug("Server sending response YAML\n--------------------------------\n%s\n--------------------------------\n", yaml);

	// clear marshalled lines but keep capturing
	plist_remove_all_free(operation->log_cap_lines);

	if (!yaml) {
		operation->done = true;
		return;
	}

	if (socket_write(operation->socket_client, yaml, strlen(yaml)) == -1) {
		operation->done = true;
	}

	free(yaml);
}

static char *ipc_receive_raw(int socket_client) {
	char *yaml = NULL;

	if (!(yaml = socket_read(socket_client))) {
		close(socket_client);
		return NULL;
	}

	return yaml;
}

struct IpcRequest *ipc_receive_request(int socket_server) {
	struct IpcRequest *request = NULL;
	int socket_client = -1;
	char *yaml = NULL;

	if ((socket_client = socket_accept(socket_server)) == -1) {
		return NULL;
	}

	if (!(yaml = ipc_receive_raw(socket_client))) {
		return NULL;
	}

	log_debug("Server received request YAML\n--------------------------------\n%s\n--------------------------------\n", yaml);

	request = yaml_unmarshal_str(yaml, yaml_root_to_ipc_request, "ipc request");
	free(yaml);

	if (!request) {
		return ipc_request_init(0);
	}

	request->socket_client = socket_client;

	return request;
}

struct Plist *ipc_receive_responses(int socket_client, char **yaml) {
	if (!(*yaml = ipc_receive_raw(socket_client))) {
		return NULL;
	}

	// --yaml sets log level to warn hence this won't be printed
	log_debug("Client received response YAML\n================================\n%s\n================================\n", *yaml);

	return yaml_unmarshal_str(*yaml, yaml_root_to_ipc_response_plist, "ipc response");
}

void ipc_request_free(struct IpcRequest *request) {
	if (!request) {
		return;
	}

	cfg_free(request->cfg);

	free(request);
}

void ipc_response_free(struct IpcResponse *response) {
	if (!response)
		return;

	cfg_free(response->cfg);
	lid_free(response->lid);
	plist_free_vals(response->heads);

	plist_free_vals(response->log_cap_lines);

	free(response);
}

void ipc_operation_destroy(struct IpcOperation *operation) {
	if (!operation)
		return;

	ipc_request_free(operation->request);

	log_cap_lines_stop(operation->log_cap_lines);

	plist_free_vals(operation->log_cap_lines);

	free(operation);
}

