# way-displays IPC

way-displays server creates a socket `${XDG_RUNTIME_DIR}/way-displays.${XDG_VTNR}.sock`. If `XDG_RUNTIME_DIR` is unset the socket will be `/tmp/way-displays.${XDG_VTNR}.sock`.

Clients send an `!!ipc_request` and will receive multiple `!!ipc_response` until the operation is complete and the socket closed.

`MESSAGES` will contain log messages by threshold, with human readable messages as written by the server. These are intended to be streamed to the user.

`DONE` will be set when the operation is complete.

`RC` will be nonzero on failure when complete.

## CFG_GET

Retrieves the current configuration along with human readable messages describing the configuration and current state.

Client sends an [!!ipc_request](#ipc_request) with [!!ipc_operation](#ipc_operation) `CFG_GET` and an empty [!!cfg](#cfg).

## CFG_WRITE

Persists the current configuration to the active `cfg.yaml`.

Client sends an [!!ipc_request](#ipc_request) with [!!ipc_operation](#ipc_operation) `CFG_WRITE` and an empty [!!cfg](#cfg).

## CFG_SET

Configure multiple elements.

Client sends an [!!ipc_request](#ipc_request) with [!!ipc_operation](#ipc_operation) `CFG_SET`.

[!!cfg](#cfg) may only contain:
- `ARRANGE_ALIGN`
- `ORDER`
- `AUTO_SCALE`
- `SCALE`
- `MODE`
- `DISABLED`

## CFG_DEL

Remove multiple elements.

Client sends an [!!ipc_request](#ipc_request) with [!!ipc_operation](#ipc_operation) `CFG_DEL`.

[!!cfg](#cfg) may only contain:
- `SCALE`
- `MODE`
- `DISABLED`

