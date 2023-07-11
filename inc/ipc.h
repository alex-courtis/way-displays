#ifndef IPC_H
#define IPC_H

#include <stdbool.h>

#define IPC_RC_SUCCESS 0
#define IPC_RC_WARN 1
#define IPC_RC_ERROR 2
#define IPC_RC_BAD_REQUEST 11
#define IPC_RC_BAD_RESPONSE 12
#define IPC_RC_REQUEST_IN_PROGRESS 13

enum IpcRequestOperation {
	GET = 1,
	CFG_SET,
	CFG_DEL,
	CFG_WRITE,
};

struct IpcOperation {
	int socket_client;
	bool done;
	int rc;
	bool send_logs;
	bool send_state;
};

struct IpcRequest {
	enum IpcRequestOperation op;
	struct Cfg *cfg;
	int socket_client;
	bool bad;
	bool raw;
};

struct IpcResponse {
	bool done;
	int rc;
	struct Cfg *cfg;
	struct SList *heads;
	struct Lid *lid;
	// TODO actually capture log lines
};

void ipc_send_request(struct IpcRequest *request);

void ipc_send_response(struct IpcOperation *operation);

char *ipc_receive_raw_client(int socket_client);

struct IpcRequest *ipc_receive_request_server(int socket_server);

struct IpcResponse *ipc_receive_response_client(int socket_client);

void ipc_request_free(struct IpcRequest *request);

void ipc_response_free(struct IpcResponse *response);

void ipc_operation_free(struct IpcOperation *operation);

#endif // IPC_H

