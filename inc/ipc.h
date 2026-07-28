#ifndef IPC_H
#define IPC_H

#include <stdbool.h>

#include "enum.h"

struct IpcOperation {
	struct IpcRequest *request;
	int socket_client;
	bool done;
	int rc;
	bool send_state;	// not for bad requests
	const struct Plist *log_cap_lines;
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
	const struct Pset *heads;
	struct Lid *lid;
	const struct Plist *log_cap_lines;
};

struct IpcOperation *ipc_operation_init(void);

struct IpcRequest *ipc_request_init(const enum IpcCommand command);

struct IpcResponse *ipc_response_init(void);

const struct Pset *ipc_response_pset_init(void);

void ipc_operation_update_rc(struct IpcOperation *ipc_operation);

void ipc_send_request(struct IpcRequest *request);

void ipc_send_operation(struct IpcOperation *operation);

// receive the entire request sent to the server socket
struct IpcRequest *ipc_receive_request(int socket_server);

// receive all responses, user frees complete yaml
struct Pset *ipc_receive_responses(int socket_client, char **yaml);

void ipc_request_free(struct IpcRequest *request);

void ipc_response_free(struct IpcResponse *response);

void ipc_operation_destroy(struct IpcOperation *operation);

#endif // IPC_H

