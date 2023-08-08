# way-displays IPC

`$WAYLAND_DISPLAY` must be set and `$XDG_VTNR` set to match the server's.

way-displays server creates a socket `${XDG_RUNTIME_DIR}/way-displays.${XDG_VTNR}.sock`.

If `$XDG_RUNTIME_DIR` is unset the socket will be `/tmp/way-displays.${XDG_VTNR}.sock`.

Clients send an [!!ipc_request](YAML_SCHEMAS.md#ipc_request) and will receive a sequence of [!!ipc_response](YAML_SCHEMAS.md#ipc_response) until the operation is complete and the socket closed.

## Example

```sh
make examples
```

### client-get-raw

Sends a `GET` and prints the raw response.

### client-set-scaling

Sends a `CFG_SET` to reset `SCALING`. Unpacks all responses and prints some config and head states as the operation progresses.

### poke-server

Validates and prints client and server readiness for IPC.

## Response

[STATE](YAML_SCHEMAS.md#state) contains the device states.

[CFG](YAML_SCHEMAS.md#cfg) contains the active configuration.

`MESSAGES` contains human readable messages by [!!log_threshold](YAML_SCHEMAS.md#log_threshold) as written by the server. These are intended to be streamed to the user.

`DONE` will be set when the operation is complete.

[RC](YAML_SCHEMAS.md#cfg) 0 until `DONE`.

## Request

An [!!ipc_request](YAML_SCHEMAS.md#ipc_request) must contain one [COMMAND](YAML_SCHEMAS.md#ipc_command).

### GET

Retrieve [CFG](YAML_SCHEMAS.md#cfg) and [STATE](YAML_SCHEMAS.md#state)

```yaml
OP: GET
```

Only one response will be sent and it will not be in a sequence.

### CFG_WRITE

Persists the active configuration to `cfg.yaml`:

```yaml
OP: CFG_WRITE
```

### CFG_SET

Mutate multiple configuration values.

Compose a [CFG](YAML_SCHEMAS.md#cfg) containing only the desired elements to change e.g.:

```yaml
OP: CFG_SET
CFG:
  SCALING: OFF
  ORDER:
    - "one"
    - "two"
```

### CFG_DEL

Remove multiple configuration values.

Only these [CFG](YAML_SCHEMAS.md#cfg) elements that may be deleted:
- `SCALE`
- `MODE`
- `DISABLED`
- `VRR_OFF`

Compose a [CFG](YAML_SCHEMAS.md#cfg) containing only the desired elements to delete e.g.:

```yaml
OP: CFG_DEL
CFG:
  SCALE:
    - NAME_DESC: DEF 456
      SCALE: 1
  MODE:
    - NAME_DESC: GHI 789
  DISABLED:
    - PQR 678
    - STU 901
    - eDP-1
```

