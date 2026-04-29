#include <stdbool.h>

#include "yaml-marshal.h"

#include "convert.h"
#include "ipc.h"
#include "log.h"
#include "yaml-marshal-cfg.h"

static bool map_ipc_request(const void *data, int mapping) {
	if (!data)
		return false;

	const struct IpcRequest *ipc_request = data;

	const char *op_name = ipc_command_name(ipc_request->command);
	if (!op_name) {
		log_error("unable to marshal ipc request: missing OP");
		return false;
	}

	map_key_to_str("OP", ipc_command_name(ipc_request->command), mapping);

	if (ipc_request->log_threshold)
		map_key_to_str("LOG_THRESHOLD", log_threshold_name(ipc_request->log_threshold), mapping);

	map_key_to_map("CFG", ipc_request->cfg, map_cfg, mapping);

	return true;
}

char *marshal_ipc_request_2(const struct IpcRequest *ipc_request) {
	return marshal_yaml_map(ipc_request, map_ipc_request, "ipc request");
}

