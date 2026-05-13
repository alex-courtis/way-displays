#include <stdbool.h>
#include <stddef.h>
#include <yaml.h>

#include "yaml-marshal.h"

#include "convert.h"
#include "ipc.h"
#include "log.h"
#include "yaml/marshal-primitives.h"
#include "yaml/marshal-types.h"

char *ipc_request_to_yaml(const struct IpcRequest *ipc_request) {
	return struct_to_yaml(ipc_request, yaml_doc_ipc_request, "ipc request");
}
