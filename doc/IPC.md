# way-displays IPC

`$WAYLAND_DISPLAY` must be set.

way-displays server creates a socket `${XDG_RUNTIME_DIR}/way-displays.${XDG_VTNR}.sock`.

If `$XDG_RUNTIME_DIR` is unset the socket will be `/tmp/way-displays.${XDG_VTNR}.sock`.

Clients send an [!!ipc_request](YAML_SCHEMAS.md#ipc_request) and will receive a sequence of [!!ipc_response](YAML_SCHEMAS.md#ipc_response) until the operation is complete and the socket closed.

See [example_client.c](../examples/example_client.c) for a standalone client that demonstrates each of the requests: `make example-client`

## Response

[STATE](YAML_SCHEMAS.md#state) contains the device states.

[CFG](YAML_SCHEMAS.md#cfg) contains the active configuration.

`MESSAGES` contains human readable messages by [!!log_threshold](YAML_SCHEMAS.md#log_threshold) as written by the server. These are intended to be streamed to the user.

`DONE` will be set when the operation is complete.

[RC](YAML_SCHEMAS.md#cfg) 0 until `DONE`.

## Request

An [!!ipc_request](YAML_SCHEMAS.md#ipc_request) must contain one [COMMAND](YAML_SCHEMAS.md#ipc_command).

### GET

Retrieves `CFG` and `STATE`.

example request:
```yaml
OP: GET
```

<details><summary>Example Response</summary><br>

```yaml
- DONE: TRUE
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
          VRR: FALSE
          MODE:
            WIDTH: 13
            HEIGHT: 14
            REFRESH_MHZ: 15
            PREFERRED: FALSE
        DESIRED:
          SCALE: 2
          ENABLED: FALSE
          X: 0
          Y: 0
          VRR: TRUE
          MODE:
            WIDTH: 13
            HEIGHT: 14
            REFRESH_MHZ: 15
            PREFERRED: TRUE
        MODES:
          - WIDTH: 2560
            HEIGHT: 1440
            REFRESH_MHZ: 59998
            PREFERRED: TRUE
            CURRENT: TRUE
  MESSAGES:
    - INFO: ""
    - INFO: "Server received request: get"
    - INFO: ""
    - INFO: "Active configuration:"
    - INFO: "  Arrange in a COLUMN aligned at the RIGHT"
    - INFO: "  Order:"
    - INFO: "    eDP-1"
    - INFO: "    ABC 123"
    - INFO: "  Auto scale: ON"
    - INFO: "  Scale:"
    - INFO: "    ASUS XG32V: 1.000"
    - INFO: "    Unknown 0x05EF: 2.000"
    - INFO: "    MAG274QRF-QD CA8A271A00352: 1.000"
    - INFO: "    DEF 456: 3.500"
    - INFO: "    HDMI-1: 1.000"
    - INFO: "  Mode:"
    - INFO: "    MAG274QRF-QD CA8A271A00352: 2560x1440"
    - INFO: "    GHI 789: 100x50@88Hz"
    - INFO: "    JKL 012: 200x100"
    - INFO: "    MNO 345: MAX"
    - INFO: "  Disabled:"
    - INFO: "    PQR 678"
    - INFO: "    STU 901"
    - INFO: "    eDP-1"
    - INFO: ""
    - INFO: "eDP-1:"
    - INFO: "  info:"
    - INFO: "    name:     'eDP-1'"
    - INFO: "    desc:     'Unknown 0x05EF 0x00000000 (eDP-1)'"
    - INFO: "    width:    310mm"
    - INFO: "    height:   170mm"
    - INFO: "    dpi:      212.45 @ 2560x1440"
    - INFO: "    mode:     2560 x 1440 @  60 Hz   59,998 mHz (preferred)"
    - INFO: "  current:"
    - INFO: "    mode:     2560x1440@60Hz (59,998mHz) (preferred)"
    - INFO: "    (disabled)"
```
</details>

### CFG_WRITE

Persists the active configuration to `cfg.yaml`.

Example Request:
```yaml
OP: CFG_WRITE
```

<details><summary>Example Response</summary><br>

```yaml
- DONE: TRUE
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
    - INFO: ""
    - INFO: "Server received request: write"
    - INFO: ""
    - INFO: "Wrote configuration file: /home/alex/.dotfiles/config/way-displays/cfg.yaml"
```
</details>

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

<details><summary>Example Response Stream</summary><br>

```yaml
- DONE: FALSE
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
    - INFO: ""
    - INFO: "Server received request: set"
    - INFO: "  Arrange in a COLUMN aligned at the RIGHT"
    - INFO: "  Order:"
    - INFO: "    eDP-1"
    - INFO: "    ABC 123"
    - INFO: "  Auto scale: ON"
    - INFO: "  Scale:"
    - INFO: "    DEF 456: 3.500"
    - INFO: "    HDMI-1: 1.000"
    - INFO: "  Mode:"
    - INFO: "    GHI 789: 100x50@88Hz"
    - INFO: "    JKL 012: 200x100"
    - INFO: "    MNO 345: MAX"
    - INFO: "  Disabled:"
    - INFO: "    PQR 678"
    - INFO: "    STU 901"
    - INFO: "    eDP-1"
    - INFO: "  Laptop display prefix: FFF"
    - INFO: ""
    - INFO: "New configuration:"
    - INFO: "  Arrange in a COLUMN aligned at the RIGHT"
    - INFO: "  Order:"
    - INFO: "    eDP-1"
    - INFO: "    ABC 123"
    - INFO: "  Auto scale: ON"
    - INFO: "  Scale:"
    - INFO: "    ASUS XG32V: 1.000"
    - INFO: "    Unknown 0x05EF: 2.000"
    - INFO: "    MAG274QRF-QD CA8A271A00352: 1.000"
    - INFO: "    DEF 456: 3.500"
    - INFO: "    HDMI-1: 1.000"
    - INFO: "  Mode:"
    - INFO: "    MAG274QRF-QD CA8A271A00352: 2560x1440"
    - INFO: "    GHI 789: 100x50@88Hz"
    - INFO: "    JKL 012: 200x100"
    - INFO: "    MNO 345: MAX"
    - INFO: "  Disabled:"
    - INFO: "    PQR 678"
    - INFO: "    STU 901"
    - INFO: "    eDP-1"
```

```yaml
- DONE: FALSE
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
    - INFO: ""
    - INFO: "eDP-1 Changing:"
    - INFO: "  from:"
    - INFO: "    scale:    2.000"
    - INFO: "    position: 0,0"
    - INFO: "    mode:     2560x1440@60Hz (59,998mHz) (preferred)"
    - INFO: "  to:"
    - INFO: "    (disabled)"
```

```yaml
- DONE: TRUE
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
    - INFO: ""
    - INFO: Changes successful
```
</details>

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
    - eDP-1
```

<details><summary>Example Response Stream</summary><br>

```yaml
- DONE: FALSE
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
    - INFO: ""
    - INFO: "Server received request: delete"
    - INFO: "  Scale:"
    - INFO: "    DEF 456"
    - INFO: "  Mode:"
    - INFO: "    GHI 789"
    - INFO: "  Disabled:"
    - INFO: "    PQR 678"
    - INFO: "    STU 901"
    - INFO: "    eDP-1"
    - INFO: ""
    - INFO: "New configuration:"
    - INFO: "  Arrange in a COLUMN aligned at the RIGHT"
    - INFO: "  Order:"
    - INFO: "    eDP-1"
    - INFO: "    ABC 123"
    - INFO: "  Auto scale: ON"
    - INFO: "  Scale:"
    - INFO: "    ASUS XG32V: 1.000"
    - INFO: "    Unknown 0x05EF: 2.000"
    - INFO: "    MAG274QRF-QD CA8A271A00352: 1.000"
    - INFO: "    HDMI-1: 1.000"
    - INFO: "  Mode:"
    - INFO: "    MAG274QRF-QD CA8A271A00352: 2560x1440"
    - INFO: "    JKL 012: 200x100"
    - INFO: "    MNO 345: MAX"
```

```yaml
- DONE: FALSE
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
    - INFO: ""
    - INFO: "eDP-1 Changing:"
    - INFO: "  from:"
    - INFO: "    mode:     2560x1440@60Hz (59,998mHz) (preferred)"
    - INFO: "    (disabled)"
    - INFO: "  to:"
    - INFO: "    scale:    2.000"
    - INFO: "    position: 0,0"
    - INFO: "    (enabled)"
```

```yaml
- DONE: TRUE
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
    - INFO: ""
    - INFO: Changes successful
```
</details>

