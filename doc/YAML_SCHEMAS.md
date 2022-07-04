# way-displays YAML Schemas

Document loosely follow the YAML [failsafe + JSON](https://yaml.org/spec/1.2.2/#chapter-10-recommended-schemas) schema.

## Enums

### `!!arrange`

`!!str` `<ROW | COL>`

### `!!align`

`!!str` `<TOP | MIDDLE | BOTTOM | LEFT | RIGHT>`

### `!!logthreshold`

`!!str` `<ERROR | WARNING | INFO | DEBUG>`

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
LOG_THRESHOLD: !!logthreshold
LAPTOP_DISPLAY_PREFIX: !!str
```

## !!cfg_get

```yaml
!!map
CFG_GET: !!map
```

Map must be empty.

## !!cfg_write

```yaml
!!map
CFG_WRITE: !!map
```

Map must be empty.

## !!cfg_set

```yaml
!!map
CFG_SET: !!map
  !!cfg
```

May only include:
- `ARRANGE_ALIGN`
- `ORDER`
- `AUTO_SCALE`
- `SCALE`
- `MODE`
- `DISABLED`

## !!cfg_del

```yaml
!!map
CFG_DEL: !!map
  !!cfg
```

May only include:
- `SCALE`
- `MODE`
- `DISABLED`

## !!response

```yaml
!!map
DONE: !!bool
MESSAGES: !!seq
  - !!map
    !!logthreshold: !!str
RC: !!int
```

