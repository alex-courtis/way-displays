# way-displays IPC

way-displays server creates a socket `${XDG_RUNTIME_DIR}/way-displays.${XDG_VTNR}.sock`. If `XDG_RUNTIME_DIR` is unset the socket will be `/tmp/way-displays.${XDG_VTNR}.sock`.

Clients send an [!!ipc_request](YAML_SCHEMAS.md#ipc_request) and will receive [!!ipc_response](YAML_SCHEMAS.md#ipc_response) until the operation is complete and the socket closed.

## Response

When the request sets `HUMAN`, the response will contain log `MESSAGES` by [!!log_threshold](YAML_SCHEMAS.md#log_threshold), with human readable messages as written by the server. These are intended to be streamed to the user.

[STATE](YAML_SCHEMAS.md#state) contains the device states.

[CFG](YAML_SCHEMAS.md#cfg) contains the active configuration.

`DONE` will be set when the operation is complete.

`RC` will be nonzero on failure when complete.

## Request

An [!!ipc_request](YAML_SCHEMAS.md#ipc_request) must contain one [COMMAND](YAML_SCHEMAS.md#ipc_command)

Other fields are only used by certain commands.

### STATE_GET

Retrieves the current state of devices.

### CFG_GET

Retrieves the active configuration.

### CFG_WRITE

Persists the active configuration to `cfg.yaml`.

### CFG_SET

Add or change multiple configuration values.

[CFG](YAML_SCHEMAS.md#cfg) elements that may be set:
- `ARRANGE_ALIGN`
- `ORDER`
- `AUTO_SCALE`
- `SCALE`
- `MODE`
- `DISABLED`

### CFG_DEL

Remove multiple configuration values.

[CFG](YAML_SCHEMAS.md#cfg) elements that may be deleted:
- `SCALE`
- `MODE`
- `DISABLED`

