# way-displays IPC

way-displays server creates a socket `${XDG_RUNTIME_DIR}/way-displays.${XDG_VTNR}.sock`. If `XDG_RUNTIME_DIR` is unset the socket will be `/tmp/way-displays.${XDG_VTNR}.sock`.

Clients send an [!!ipc_request](YAML_SCHEMAS.md#ipc_request) and will receive [!!ipc_response](YAML_SCHEMAS.md#ipc_response) until the operation is complete and the socket closed.

See [example_client.c](../examples/example_client.c) for a standalone client that demonstrates each of the requests: `make example-client`

## Response

When the request sets `HUMAN`, the response will contain log `MESSAGES` by [!!log_threshold](YAML_SCHEMAS.md#log_threshold), with human readable messages as written by the server. These are intended to be streamed to the user.

[STATE](YAML_SCHEMAS.md#state) contains the device states.

[CFG](YAML_SCHEMAS.md#cfg) contains the active configuration.

`DONE` will be set when the operation is complete.

`RC` will be nonzero on failure when complete.

## Request

An [!!ipc_request](YAML_SCHEMAS.md#ipc_request) must contain one [COMMAND](YAML_SCHEMAS.md#ipc_command).

Unless stated otherwise, `STATE` and `CFG` will be populated in the response.

### ALL_GET

Retrieves `CFG` and `STATE`.

example request:
```yaml
OP: ALL_GET
HUMAN: TRUE
```

### STATE_GET

Retrieves `STATE`.

example request:
```yaml
OP: STATE_GET
HUMAN: TRUE
```

example response:
```yaml
DONE: TRUE
RC: 0
STATE:
  LID:
    CLOSED: FALSE
    DEVICE_PATH: /dev/input/event1
  HEADS:
    - NAME: eDP-1
      DESCRIPTION: Unknown 0x05EF 0x00000000 (eDP-1)
      WIDTH_MM: 310
      HEIGHT_MM: 170
      TRANSFORM: 0
      MAKE: Unknown
      MODEL: 0x05EF
      SERIAL_NUMBER: 0x00000000
      CURRENT:
        SCALE: 2
        ENABLED: TRUE
        X: 0
        Y: 0
      DESIRED:
        SCALE: 2
        ENABLED: TRUE
        X: 0
        Y: 0
      MODES:
        - WIDTH: 2560
          HEIGHT: 1440
          REFRESH_MHZ: 59998
          PREFERRED: TRUE
          CURRENT: TRUE
MESSAGES:
  INFO: ""
  INFO: "eDP-1:"
  INFO: "  info:"
  INFO: "    name:     'eDP-1'"
  INFO: "    desc:     'Unknown 0x05EF 0x00000000 (eDP-1)'"
  INFO: "    width:    310mm"
  INFO: "    height:   170mm"
  INFO: "    dpi:      212.45 @ 2560x1440"
  INFO: "    mode:     2560 x 1440 @  60 Hz   59,998 mHz (preferred)"
  INFO: "  current:"
  INFO: "    scale:    2.000"
  INFO: "    position: 0,0"
  INFO: "    mode:     2560x1440@60Hz (59,998mHz) (preferred)"
```

### CFG_GET

Retrieves `CFG`.

example request:
```yaml
OP: CFG_GET
HUMAN: TRUE
```

example response:
```yaml
DONE: TRUE
RC: 0
CFG:
  ARRANGE: ROW
  ALIGN: BOTTOM
  ORDER:
    - Sharp Corporation 0x148D
    - Unknown 0x05EF
    - ASUS XG32V
    - Ancor Communications Inc ROG PG27AQ
  AUTO_SCALE: TRUE
  SCALE:
    - NAME_DESC: ASUS XG32V
      SCALE: 1
    - NAME_DESC: Unknown 0x05EF
      SCALE: 2
    - NAME_DESC: MAG274QRF-QD CA8A271A00352
      SCALE: 1
  MODE:
    - NAME_DESC: MAG274QRF-QD CA8A271A00352
      WIDTH: 2560
      HEIGHT: 1440
MESSAGES:
  INFO: ""
  INFO: "Active configuration:"
  INFO: "  Arrange in a ROW aligned at the BOTTOM"
  INFO: "  Order:"
  INFO: "    Sharp Corporation 0x148D"
  INFO: "    Unknown 0x05EF"
  INFO: "    ASUS XG32V"
  INFO: "    Ancor Communications Inc ROG PG27AQ"
  INFO: "  Auto scale: ON"
  INFO: "  Scale:"
  INFO: "    ASUS XG32V: 1.000"
  INFO: "    Unknown 0x05EF: 2.000"
  INFO: "    MAG274QRF-QD CA8A271A00352: 1.000"
  INFO: "  Mode:"
  INFO: "    MAG274QRF-QD CA8A271A00352: 2560x1440"
```

### CFG_WRITE

Persists the active configuration to `cfg.yaml`.

example request:
```yaml
OP: CFG_WRITE
HUMAN: TRUE
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
HUMAN: TRUE
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

example responses:
```yaml
DONE: FALSE
RC: 0
CFG:
  ARRANGE: COLUMN
  ALIGN: RIGHT
  ORDER:
    - eDP-1
    - ABC 123
  AUTO_SCALE: TRUE
  SCALE:
    - NAME_DESC: ASUS XG32V
      SCALE: 1
    - NAME_DESC: Unknown 0x05EF
      SCALE: 2
    - NAME_DESC: MAG274QRF-QD CA8A271A00352
      SCALE: 1
    - NAME_DESC: DEF 456
      SCALE: 3.5
    - NAME_DESC: HDMI-1
      SCALE: 1
  MODE:
    - NAME_DESC: MAG274QRF-QD CA8A271A00352
      WIDTH: 2560
      HEIGHT: 1440
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
STATE:
  LID:
    CLOSED: FALSE
    DEVICE_PATH: /dev/input/event1
  HEADS:
    - NAME: eDP-1
      DESCRIPTION: Unknown 0x05EF 0x00000000 (eDP-1)
      WIDTH_MM: 310
      HEIGHT_MM: 170
      TRANSFORM: 0
      MAKE: Unknown
      MODEL: 0x05EF
      SERIAL_NUMBER: 0x00000000
      CURRENT:
        SCALE: 2
        ENABLED: TRUE
        X: 0
        Y: 0
      DESIRED:
        SCALE: 2
        ENABLED: TRUE
        X: 0
        Y: 0
      MODES:
        - WIDTH: 2560
          HEIGHT: 1440
          REFRESH_MHZ: 59998
          PREFERRED: TRUE
          CURRENT: TRUE
MESSAGES:
  INFO: ""
  INFO: "Applying new configuration:"
  INFO: ""
  INFO: "Active configuration:"
  INFO: "  Arrange in a COLUMN aligned at the RIGHT"
  INFO: "  Order:"
  INFO: "    eDP-1"
  INFO: "    ABC 123"
  INFO: "  Auto scale: ON"
  INFO: "  Scale:"
  INFO: "    ASUS XG32V: 1.000"
  INFO: "    Unknown 0x05EF: 2.000"
  INFO: "    MAG274QRF-QD CA8A271A00352: 1.000"
  INFO: "    DEF 456: 3.500"
  INFO: "    HDMI-1: 1.000"
  INFO: "  Mode:"
  INFO: "    MAG274QRF-QD CA8A271A00352: 2560x1440"
  INFO: "    GHI 789: 100x50@88Hz"
  INFO: "    JKL 012: 200x100"
  INFO: "    MNO 345: MAX"
  INFO: "  Disabled:"
  INFO: "    PQR 678"
  INFO: "    STU 901"
  INFO: "    eDP-1"
```

```yaml
DONE: TRUE
RC: 0
CFG:
  ARRANGE: COLUMN
  ALIGN: RIGHT
  ORDER:
    - eDP-1
    - ABC 123
  AUTO_SCALE: TRUE
  SCALE:
    - NAME_DESC: ASUS XG32V
      SCALE: 1
    - NAME_DESC: Unknown 0x05EF
      SCALE: 2
    - NAME_DESC: MAG274QRF-QD CA8A271A00352
      SCALE: 1
    - NAME_DESC: DEF 456
      SCALE: 3.5
    - NAME_DESC: HDMI-1
      SCALE: 1
  MODE:
    - NAME_DESC: MAG274QRF-QD CA8A271A00352
      WIDTH: 2560
      HEIGHT: 1440
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
STATE:
  LID:
    CLOSED: FALSE
    DEVICE_PATH: /dev/input/event1
  HEADS:
    - NAME: eDP-1
      DESCRIPTION: Unknown 0x05EF 0x00000000 (eDP-1)
      WIDTH_MM: 310
      HEIGHT_MM: 170
      TRANSFORM: 0
      MAKE: Unknown
      MODEL: 0x05EF
      SERIAL_NUMBER: 0x00000000
      CURRENT:
        SCALE: 2
        ENABLED: FALSE
        X: 0
        Y: 0
      DESIRED:
        SCALE: 2
        ENABLED: FALSE
        X: 0
        Y: 0
      MODES:
        - WIDTH: 2560
          HEIGHT: 1440
          REFRESH_MHZ: 59998
          PREFERRED: TRUE
          CURRENT: TRUE
MESSAGES:
  INFO: ""
  INFO: "eDP-1 Changing:"
  INFO: "  from:"
  INFO: "    scale:    2.000"
  INFO: "    position: 0,0"
  INFO: "    mode:     2560x1440@60Hz (59,998mHz) (preferred)"
  INFO: "  to:"
  INFO: "    (disabled)"
  INFO: ""
  INFO: Changes successful
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
HUMAN: TRUE
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

example responses:
```yaml
DONE: FALSE
RC: 0
CFG:
  ARRANGE: COLUMN
  ALIGN: RIGHT
  ORDER:
    - eDP-1
    - ABC 123
  AUTO_SCALE: TRUE
  SCALE:
    - NAME_DESC: ASUS XG32V
      SCALE: 1
    - NAME_DESC: Unknown 0x05EF
      SCALE: 2
    - NAME_DESC: MAG274QRF-QD CA8A271A00352
      SCALE: 1
    - NAME_DESC: HDMI-1
      SCALE: 1
  MODE:
    - NAME_DESC: MAG274QRF-QD CA8A271A00352
      WIDTH: 2560
      HEIGHT: 1440
    - NAME_DESC: JKL 012
      WIDTH: 200
      HEIGHT: 100
    - NAME_DESC: MNO 345
      MAX: TRUE
  DISABLED:
    - eDP-1
STATE:
  LID:
    CLOSED: FALSE
    DEVICE_PATH: /dev/input/event1
  HEADS:
    - NAME: eDP-1
      DESCRIPTION: Unknown 0x05EF 0x00000000 (eDP-1)
      WIDTH_MM: 310
      HEIGHT_MM: 170
      TRANSFORM: 0
      MAKE: Unknown
      MODEL: 0x05EF
      SERIAL_NUMBER: 0x00000000
      CURRENT:
        SCALE: 2
        ENABLED: FALSE
        X: 0
        Y: 0
      DESIRED:
        SCALE: 2
        ENABLED: FALSE
        X: 0
        Y: 0
      MODES:
        - WIDTH: 2560
          HEIGHT: 1440
          REFRESH_MHZ: 59998
          PREFERRED: TRUE
          CURRENT: TRUE
MESSAGES:
  INFO: ""
  INFO: "Applying new configuration:"
  INFO: ""
  INFO: "Active configuration:"
  INFO: "  Arrange in a COLUMN aligned at the RIGHT"
  INFO: "  Order:"
  INFO: "    eDP-1"
  INFO: "    ABC 123"
  INFO: "  Auto scale: ON"
  INFO: "  Scale:"
  INFO: "    ASUS XG32V: 1.000"
  INFO: "    Unknown 0x05EF: 2.000"
  INFO: "    MAG274QRF-QD CA8A271A00352: 1.000"
  INFO: "    HDMI-1: 1.000"
  INFO: "  Mode:"
  INFO: "    MAG274QRF-QD CA8A271A00352: 2560x1440"
  INFO: "    JKL 012: 200x100"
  INFO: "    MNO 345: MAX"
  INFO: "  Disabled:"
  INFO: "    eDP-1"
```

```yaml
DONE: TRUE
RC: 0
CFG:
  ARRANGE: COLUMN
  ALIGN: RIGHT
  ORDER:
    - eDP-1
    - ABC 123
  AUTO_SCALE: TRUE
  SCALE:
    - NAME_DESC: ASUS XG32V
      SCALE: 1
    - NAME_DESC: Unknown 0x05EF
      SCALE: 2
    - NAME_DESC: MAG274QRF-QD CA8A271A00352
      SCALE: 1
    - NAME_DESC: HDMI-1
      SCALE: 1
  MODE:
    - NAME_DESC: MAG274QRF-QD CA8A271A00352
      WIDTH: 2560
      HEIGHT: 1440
    - NAME_DESC: JKL 012
      WIDTH: 200
      HEIGHT: 100
    - NAME_DESC: MNO 345
      MAX: TRUE
  DISABLED:
    - eDP-1
STATE:
  LID:
    CLOSED: FALSE
    DEVICE_PATH: /dev/input/event1
  HEADS:
    - NAME: eDP-1
      DESCRIPTION: Unknown 0x05EF 0x00000000 (eDP-1)
      WIDTH_MM: 310
      HEIGHT_MM: 170
      TRANSFORM: 0
      MAKE: Unknown
      MODEL: 0x05EF
      SERIAL_NUMBER: 0x00000000
      CURRENT:
        SCALE: 2
        ENABLED: FALSE
        X: 0
        Y: 0
      DESIRED:
        SCALE: 2
        ENABLED: FALSE
        X: 0
        Y: 0
      MODES:
        - WIDTH: 2560
          HEIGHT: 1440
          REFRESH_MHZ: 59998
          PREFERRED: TRUE
          CURRENT: TRUE
```

