#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ipc.h"

#include "cfg.h"
#include "convert.h"
#include "info.h"
#include "log.h"
#include "marshalling.h"
#include "sockets.h"

int ipc_request_send(struct IpcRequest *request) {
	int fd = -1;

	char *yaml = marshal_ipc_request(request);
	if (!yaml) {
		goto end;
	}

	log_debug("========sending server request==========\n%s\n----------------------------------------", yaml);
	log_info("Sending %s request:", ipc_request_command_friendly(request->command));
	print_cfg(INFO, request->cfg, request->command == CFG_DEL);

	if ((fd = create_fd_ipc_client()) == -1) {
		goto end;
	}

	if (socket_write(fd, yaml, strlen(yaml)) == -1) {
		fd = -1;
		goto end;
	}

end:
	if (yaml) {
		free(yaml);
	}

	return fd;
}

void ipc_response_send(struct IpcResponse *response) {
	char *yaml = marshal_ipc_response(response);

	if (!yaml) {
		return;
	}

	log_debug("========sending client response==========\n%s----------------------------------------", yaml);

	if (socket_write(response->fd, yaml, strlen(yaml)) == -1) {
		response->done = true;
	}

	free(yaml);

	if (response->done) {
		close(response->fd);
	} else {
		log_capture_start();
	}
}

struct IpcRequest *ipc_request_receive(int fd_sock) {
	struct IpcRequest *request = NULL;
	char *yaml = NULL;
	int fd = -1;

	if ((fd = socket_accept(fd_sock)) == -1) {
		return NULL;
	}

	if (!(yaml = socket_read(fd))) {
		close(fd);
		return NULL;
	}

	log_debug("========received client request=========\n%s\n----------------------------------------", yaml);

	log_capture_start();

	request = unmarshal_ipc_request(yaml);
	free(yaml);

	log_capture_end();

	if (!request) {
		request = (struct IpcRequest*)calloc(1, sizeof(struct IpcRequest));
		request->bad = true;
		request->fd = fd;
		return request;
	}

	request->fd = fd;

	return request;
}

struct IpcResponse *ipc_response_receive(int fd) {
	struct IpcResponse *response = NULL;
	char *yaml = NULL;

	if (fd == -1) {
		log_error("invalid fd for ipc response receive");
		goto err;
	}

	if (!(yaml = socket_read(fd))) {
		goto err;
	}

	log_debug("========received server response========\n%s\n----------------------------------------", yaml);

	response = unmarshal_ipc_response(yaml);
	free(yaml);

	return response;

err:
	log_error("\nFailed to read IPC response");

	response = (struct IpcResponse*)calloc(1, sizeof(struct IpcResponse));
	response->done = true;
	response->rc = 1;

	return response;
}

void free_ipc_request(struct IpcRequest *request) {
	if (!request) {
		return;
	}

	cfg_free(request->cfg);

	free(request);
}

void free_ipc_response(struct IpcResponse *response) {
	if (!response) {
		return;
	}

	free(response);
}
