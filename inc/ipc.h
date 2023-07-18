#ifndef IPC_H
#define IPC_H

#include <stdbool.h>

#define IPC_RC_SUCCESS 0
#define IPC_RC_WARN 1
#define IPC_RC_ERROR 2
#define IPC_RC_BAD_REQUEST 11
#define IPC_RC_BAD_RESPONSE 12
#define IPC_RC_REQUEST_IN_PROGRESS 13

enum IpcCommand {
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
	enum IpcCommand command;
	struct Cfg *cfg;
	int socket_client;
	bool bad;
	bool raw;
};

struct IpcResponseStatus {
	bool done;
	int rc;
};

struct IpcResponse {
	struct IpcResponseStatus status;
	struct Cfg *cfg;
	struct SList *heads;
	struct Lid *lid;
	// TODO actually capture log lines
};

void ipc_send_request(struct IpcRequest *request);

void ipc_send_operation(struct IpcOperation *operation);

char *ipc_receive_raw(int socket_client);

// receive the entire request sent to the server socket
struct IpcRequest *ipc_receive_request(int socket_server);

// receive the entire response
struct IpcResponse *ipc_receive_response(int socket_client);

// recieve and aggregate all responses status, logging messages
struct IpcResponseStatus *ipc_receive_responses_log(int socket_client);

void ipc_request_free(struct IpcRequest *request);

void ipc_response_free(struct IpcResponse *response);

void ipc_response_status_free(struct IpcResponseStatus *response_status);

void ipc_operation_free(struct IpcOperation *operation);

#endif // IPC_H

