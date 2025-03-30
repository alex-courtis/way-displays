#ifndef IPC_H
#define IPC_H

#include <stdbool.h>

#include "log.h"

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
	CFG_TOGGLE,
};

struct IpcOperation {
	struct IpcRequest *request;
	int socket_client;
	bool done;
	int rc;
	bool send_state;	// not for bad requests
	struct SList *log_cap_lines;
};

struct IpcRequest {
	enum IpcCommand command;
	enum LogThreshold log_threshold;	// server marshals >=
	struct Cfg *cfg;				// for CFG_SET, CFG_DEL, CFG_TOGGLE
	bool yaml;				// client print yaml only bar errors
	int socket_client;		// client and server, set to -1 on failure
	bool bad;				// used by server on receipt of bad message
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
	struct SList *log_cap_lines;
};

void ipc_send_request(struct IpcRequest *request);

void ipc_send_operation(struct IpcOperation *operation);

// receive the entire request sent to the server socket
struct IpcRequest *ipc_receive_request(int socket_server);

// receive all responses, user frees complete yaml
struct SList *ipc_receive_responses(int socket_client, char **yaml);

void ipc_request_free(struct IpcRequest *request);

void ipc_response_free(const void *response);

void ipc_operation_free(struct IpcOperation *operation);

#endif // IPC_H

