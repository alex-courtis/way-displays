#ifndef IPC_H
#define IPC_H

#include <stdbool.h>

#define IPC_RC_SUCCESS 0
#define IPC_RC_WARN 1
#define IPC_RC_ERROR 2
#define IPC_RC_BAD_REQUEST 11
#define IPC_RC_BAD_RESPONSE 12
#define IPC_RC_REQUEST_IN_PROGRESS 13

enum IpcRequestCommand {
	GET = 1,
	CFG_SET,
	CFG_DEL,
	CFG_WRITE,
};

struct IpcRequest {
	enum IpcRequestCommand command;
	struct Cfg *cfg;
	int fd;
	bool bad;
};

struct IpcResponse {
	bool done;
	int rc;
	int fd;
};

int ipc_request_send(struct IpcRequest *request);

void ipc_response_send(struct IpcResponse *response);

struct IpcRequest *ipc_request_receive(int fd_sock);

struct IpcResponse *ipc_response_receive(int fd);

void free_ipc_request(struct IpcRequest *request);

void free_ipc_response(struct IpcResponse *response);

#endif // IPC_H

