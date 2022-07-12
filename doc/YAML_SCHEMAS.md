# way-displays YAML Schemas

Document loosely follow the YAML [failsafe + JSON](https://yaml.org/spec/1.2.2/#chapter-10-recommended-schemas) schema.

## Enums

### !!arrange

`!!str` : `<ROW | COL>`

### !!align

`!!str` : `<TOP | MIDDLE | BOTTOM | LEFT | RIGHT>`

### !!log_threshold

`!!str` : `<ERROR | WARNING | INFO | DEBUG>`

### !!transform

`!!str` : `<90 | 180 | 270 | FLIPPED | FLIPPED-90 | FLIPPED-180 | FLIPPED-270>`

## !!ipc_command

`!!str` : `<GET | CFG_WRITE | CFG_SET | CFG_DEL>`

## !!cfg

Used by configuration file. See [default cfg.yaml](../cfg.yaml) for an example.

```yaml
!!map
ARRANGE: !!arrange
ALIGN: !!align
ORDER: !!seq
  - !!str
AUTO_SCALE: !!bool
SCALE: !!seq
  - !!map
    NAME_DESC: !!str
    SCALE: !!float
MODE: !!seq
  - !!map
    NAME_DESC: !!str
    WIDTH: !!int
    HEIGHT: !!int
    HZ: !!int
  - !!map
    NAME_DESC: !!str
    WIDTH: !!int
    HEIGHT: !!int
  - !!map
    NAME_DESC: !!str
    MAX: !!bool
DISABLED: !!seq
  - !!str
LOG_THRESHOLD: !!log_threshold
LAPTOP_DISPLAY_PREFIX: !!str
```

## !!lid

```yaml
!!map
CLOSED: !!bool
DEVICE_PATH: !!str
```

## !!head_state

```yaml
!!map
SCALE: !!float
ENABLED: !!bool
X: !!int
Y: !!int
???current mode???
```

## !!mode

```yaml
!!map
WIDTH: !!int
HEIGHT: !!int
REFRESH_MHZ: !!int
PREFERRED: !!bool
CURRENT: !!bool
```

## !!head

```yaml
!!map
NAME: !!str
DESCRIPTION: !!str
WIDTH_MM: !!int
HEIGHT_MM: !!int
TRANSFORM: !!int
MAKE: !!str
MODEL: !!str
SERIAL_NUMBER: !!str
CURRENT: !!head_state
DESIRED: !!head_state
MODES: !!seq
  - !!mode
```

## !!ipc_request

```yaml
!!map
COMMAND: !!ipc_command
CFG: !!cfg
```

## !!ipc_response

```yaml
!!map
DONE: !!bool
RC: !!int
STATE:
  HEADS: !!seq
  - !!head
  LID: !!lid
CFG: !!cfg
MESSAGES: !!seq
  - !!map
    !!log_threshold: !!str
```

