#ifndef IPC_H
#define IPC_H

#include <stdbool.h>

enum IpcRequestCommand {
	ALL_GET = 1,
	STATE_GET,
	CFG_GET,
	CFG_SET,
	CFG_DEL,
	CFG_WRITE,
};

struct IpcRequest {
	enum IpcRequestCommand command;
	bool human;
	struct Cfg *cfg;
	int fd;
	bool bad;
};

struct IpcResponse {
	bool human;
	bool done;
	int rc;
	struct Cfg *cfg;
	struct Lid *lid;
	struct SList *heads;
	int fd;
};

int ipc_request_send(struct IpcRequest *request);

void ipc_response_send(struct IpcResponse *response);

struct IpcRequest *ipc_request_receive(int fd_sock);

struct IpcResponse *ipc_response_receive(int fd);

void free_ipc_request(struct IpcRequest *request);

void free_ipc_response(struct IpcResponse *response);

#endif // IPC_H

