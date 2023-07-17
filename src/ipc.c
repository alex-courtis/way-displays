#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ipc.h"

#include "cfg.h"
#include "head.h"
#include "lid.h"
#include "log.h"
#include "marshalling.h"
#include "slist.h"
#include "sockets.h"

void ipc_send_request(struct IpcRequest *request) {

	char *yaml = marshal_ipc_request(request);
	if (!yaml) {
		goto end;
	}

	log_debug_nocap("========sending server request==========\n%s\n----------------------------------------", yaml);

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

void ipc_send_operation(struct IpcOperation *operation) {
	char *yaml = marshal_ipc_response(operation);

	if (!yaml) {
		operation->done = true;
		return;
	}

	log_debug_nocap("========sending client response==========\n%s----------------------------------------", yaml);

	if (socket_write(operation->socket_client, yaml, strlen(yaml)) == -1) {
		operation->done = true;
	}

	free(yaml);
}

char *ipc_receive_raw(int socket_client) {
	char *yaml = NULL;

	if (!(yaml = socket_read(socket_client))) {
		close(socket_client);
		return NULL;
	}

	return yaml;
}

struct IpcRequest *ipc_receive_request_server(int socket_server) {
	struct IpcRequest *request = NULL;
	int socket_client = -1;
	char *yaml = NULL;

	if ((socket_client = socket_accept(socket_server)) == -1) {
		return NULL;
	}

	if (!(yaml = ipc_receive_raw(socket_client))) {
		return NULL;
	}

	log_debug_nocap("========received client request=========\n%s\n----------------------------------------", yaml);

	request = unmarshal_ipc_request(yaml);
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

struct IpcResponse *ipc_receive_response_client(int socket_client) {
	struct IpcResponse *response = NULL;
	char *yaml = NULL;

	if (!(yaml = ipc_receive_raw(socket_client))) {
		return NULL;
	}

	log_debug_nocap("========received server response========\n%s\n----------------------------------------", yaml);

	response = unmarshal_ipc_response(yaml);
	free(yaml);

	return response;
}

void ipc_request_free(struct IpcRequest *request) {
	if (!request) {
		return;
	}

	cfg_free(request->cfg);

	free(request);
}

void ipc_response_free(struct IpcResponse *response) {
	if (!response) {
		return;
	}

	cfg_free(response->cfg);
	lid_free(response->lid);
	slist_free_vals(&response->heads, head_free);

	free(response);
}

void ipc_operation_free(struct IpcOperation *operation) {
	free(operation);
}

