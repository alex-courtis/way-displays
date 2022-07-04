# way-displays YAML Schemas

Document loosely follow the YAML [failsafe + JSON](https://yaml.org/spec/1.2.2/#chapter-10-recommended-schemas) schema.

## Enums

### !!arrange

`!!str` : `<ROW | COL>`

### !!align

`!!str` : `<TOP | MIDDLE | BOTTOM | LEFT | RIGHT>`

### !!log_threshold

`!!str` : `<ERROR | WARNING | INFO | DEBUG>`

## !!ipc_operation

`!!str` : `<CFG_GET | CFG_WRITE | CFG_SET | CFG_DEL>`

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

## !!ipc_request

```yaml
!!map
!!ipc_operation:
  !!cfg
```

## !!ipc_response

```yaml
!!map
DONE: !!bool
RC: !!int
CFG: !!cfg
MESSAGES: !!seq
  - !!map
    !!log_threshold: !!str
```

