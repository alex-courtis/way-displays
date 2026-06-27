#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ipc.h"

#include "cfg.h"
#include "fn.h"
#include "head.h"
#include "lid.h"
#include "log.h"
#include "slist.h"
#include "sockets.h"
#include "yaml/marshal-types.h"
#include "yaml/marshal.h"
#include "yaml/unmarshal-types.h"
#include "yaml/unmarshal.h"

void ipc_send_request(struct IpcRequest *request) {

	char *yaml = yaml_marshal(request, (fn_yaml_doc)yaml_ipc_request_to_doc, "ipc request");
	if (!yaml) {
		goto end;
	}

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

void ipc_operation_update_rc(struct IpcOperation *ipc_operation) {
	if (!ipc_operation)
		return;

	for (struct SList *i = ipc_operation->log_cap_lines; i; i = i->nex) {
		const struct LogCapLine *cap_line = (struct LogCapLine*)i->val;

		if (cap_line->threshold == WARNING && ipc_operation->rc < IPC_RC_WARN)
			ipc_operation->rc = IPC_RC_WARN;
		if (cap_line->threshold == ERROR && ipc_operation->rc < IPC_RC_ERROR)
			ipc_operation->rc = IPC_RC_ERROR;
	}
}

void ipc_send_operation(struct IpcOperation *operation) {
	ipc_operation_update_rc(operation);

	char *yaml = yaml_marshal(operation, (fn_yaml_doc)yaml_ipc_operation_to_doc, "ipc response");

	log_cap_lines_free(&operation->log_cap_lines);

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

	request = yaml_unmarshal_str(yaml, yaml_root_to_ipc_request, "ipc request");
	free(yaml);

	if (!request) {
		request = (struct IpcRequest*)calloc(1, sizeof(struct IpcRequest));
		request->bad = true;
		request->socket_client = socket_client;
		return request;
	}

	request->socket_client = socket_client;

	return request;
}

struct SList *ipc_receive_responses(int socket_client, char **yaml) {
	if (!(*yaml = ipc_receive_raw(socket_client))) {
		return NULL;
	}

	struct SList *responses = yaml_unmarshal_str(*yaml, yaml_root_to_ipc_response_list, "ipc response");

	return responses;
}

void ipc_request_free(struct IpcRequest *request) {
	if (!request) {
		return;
	}

	cfg_free(request->cfg);

	free(request);
}

void ipc_response_free(struct IpcResponse *response) {
	cfg_free(response->cfg);
	lid_free(response->lid);
	slist_free_vals(&response->heads, (fn_free)head_free);

	log_cap_lines_free(&response->log_cap_lines);

	free(response);
}

void ipc_operation_free(struct IpcOperation *operation) {
	if (!operation)
		return;

	ipc_request_free(operation->request);

	log_cap_lines_stop(&operation->log_cap_lines);

	log_cap_lines_free(&operation->log_cap_lines);

	free(operation);
}

