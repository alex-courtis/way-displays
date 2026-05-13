#include <stdio.h>
#include <stdbool.h>
#include <yaml.h>

#include "yaml-marshal.h"

#include "convert.h"
#include "global.h"
#include "head.h"
#include "ipc.h"
#include "log.h"
#include "slist.h"
#include "yaml/marshal-primitives.h"
#include "yaml/marshal-types.h"

char *ipc_response_to_yaml(struct IpcOperation *ipc_operation) {
	return struct_to_yaml(ipc_operation, yaml_doc_ipc_operation, "ipc response");
}
