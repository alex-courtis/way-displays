# way-displays IPC

way-displays server creates a socket `${XDG_RUNTIME_DIR}/way-displays.${XDG_VTNR}.sock`. If `XDG_RUNTIME_DIR` is unset the socket will be `/tmp/way-displays.${XDG_VTNR}.sock`.

Clients send an [!!ipc_request](YAML_SCHEMAS.md#ipc_request) and will receive [!!ipc_response](YAML_SCHEMAS.md#ipc_response) until the operation is complete and the socket closed.

See [example_client.c](../examples/example_client.c) for a standalone client that demonstrates each of the requests: `make example-client`

## Response

[STATE](YAML_SCHEMAS.md#state) contains the device states.

[CFG](YAML_SCHEMAS.md#cfg) contains the active configuration.

`MESSAGES` contains human readable messages by [!!log_threshold](YAML_SCHEMAS.md#log_threshold) as written by the server. These are intended to be streamed to the user.

`DONE` will be set when the operation is complete.

`RC` on completion: 0 on success, see `[ipc.h](../inc/ipc.h)` for failure codes.

## Request

An [!!ipc_request](YAML_SCHEMAS.md#ipc_request) must contain one [COMMAND](YAML_SCHEMAS.md#ipc_command).

### GET

Retrieves `CFG` and `STATE`.

example request:
```yaml
OP: GET
```

example response:
```yaml
```

### CFG_WRITE

Persists the active configuration to `cfg.yaml`.

example request:
```yaml
OP: CFG_WRITE
```

example response:
```yaml
```

### CFG_SET

Add or change multiple configuration values.

[CFG](YAML_SCHEMAS.md#cfg) elements that may be set:
- `ARRANGE`
- `ALIGN`
- `ORDER`
- `AUTO_SCALE`
- `SCALE`
- `MODE`
- `DISABLED`

example request:
```yaml
OP: CFG_SET
CFG:
  ARRANGE: COL
  ALIGN: RIGHT
  ORDER:
    - eDP-1
    - ABC 123
  AUTO_SCALE: ON
  SCALE:
    - NAME_DESC: DEF 456
      SCALE: 3.5
    - NAME_DESC: HDMI-1
      SCALE: 1
  MODE:
    - NAME_DESC: GHI 789
      WIDTH: 100
      HEIGHT: 50
      HZ: 88
    - NAME_DESC: JKL 012
      WIDTH: 200
      HEIGHT: 100
    - NAME_DESC: MNO 345
      MAX: TRUE
  DISABLED:
    - PQR 678
    - STU 901
    - eDP-1
  LAPTOP_DISPLAY_PREFIX: FFF
```

example response stream:
```yaml
```

```yaml
```

### CFG_DEL

Remove multiple configuration values.

[CFG](YAML_SCHEMAS.md#cfg) elements that may be deleted:
- `SCALE`
- `MODE`
- `DISABLED`

example request:
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
```

example response stream:
```yaml
```

```yaml
```

