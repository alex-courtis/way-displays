#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ipc.h"

#include "cfg.h"
#include "log.h"
#include "marshalling.h"
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

void ipc_send_response(struct IpcResponse *response) {
	char *yaml = marshal_ipc_response(response);

	if (!yaml) {
		response->done = true;
		return;
	}

	log_debug_nocap("========sending client response==========\n%s----------------------------------------", yaml);

	if (socket_write(response->socket_client, yaml, strlen(yaml)) == -1) {
		response->done = true;
	}

	free(yaml);
}

char *ipc_receive_raw_client(int socket_client) {
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

	if (!(yaml = ipc_receive_raw_client(socket_client))) {
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

	if (!(yaml = ipc_receive_raw_client(socket_client))) {
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

	free(response);
}

