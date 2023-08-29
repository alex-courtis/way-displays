# way-displays Configuration

## On Names and Descriptions
You can configure displays by name or description. You can find these by looking at the logs e.g.

```
DP-3 Arrived:
    name:     'DP-3'
    desc:     'Unknown Monitor Maker ABC123 (DP-3 via HDMI)'
```

It is recommended to use the description rather than the name, as the name may change over time and will most likely be different on different PCs.

Any item prefixed with a `!` will be interpreted as extended POSIX regex e.g. `'!^DP-.*'`. Regex strings **must** be single quoted.

Using a regex is preferred, however partial string matches of at least 3 characters may be used.

The description does contain information about how it is connected, so don't match that. In the above example, you could use `!.*Monitor Maker ABC123.*` or `Monitor Maker ABC123`.

## cfg.yaml

See the [default cfg.yaml](../cfg.yaml), usually installed at `/etc/way-displays/cfg.yaml`.

`cfg.yaml` will be monitored for changes, which will be immediately applied.

See [YAML_SCHEMAS](YAML_SCHEMAS.md) for syntax.

The file may be specified via the `--config` command line option.

### File Locations

The following are used, in order:
* `$XDG_CONFIG_HOME/way-displays/cfg.yaml`
* `$HOME/.config/way-displays/cfg.yaml`
* `/usr/local/etc/way-displays/cfg.yaml`
* `/etc/way-displays/cfg.yaml`

### ARRANGE and ALIGN

The default is to arrange in a row, aligned at the top of the displays.

<img width="427" height="189" title="credit: Stephen Barratt" src="layouts.png?raw=true">

`ARRANGE` may be a `ROW` (left to right) or a `COLUMN` (top to bottom).

`ALIGN` for a `ROW` may be `TOP`, `MIDDLE`, `BOTTOM`.

`ALIGN` for a `COLUMN` may be `LEFT`, `MIDDLE`, `RIGHT`.

Layout to suit you e.g. top to bottom, aligned in the centre:
```yaml
# Arrange displays in a ROW (default, left to right) or a COLUMN (top to bottom)
ARRANGE: COLUMN

# Align ROWs at the TOP (default), MIDDLE or BOTTOM
# Align COLUMNs at the LEFT (default), MIDDLE or RIGHT
ALIGN: MIDDLE
```

### ORDER

`ROW` is arranged in order left to right. `COLUMN` is top to bottom. `ORDER` defaults to the order in which displays are discovered.

Define your own e.g.:
```yaml
ORDER:
    - 'DP-2'
    - 'Monitor Maker ABC123'
    - '!^my_regex_here[0-9]+'
```

Regexes are encouraged, allowing for one to easily create generic rules e.g. `'!^DP-'`, which will often be sufficient to put external monitors at the top of a column.

Three passes will be made over ORDER to match displays:
1. Exact match
1. Regex match
1. Partial match
Remaining displays will be used in their discovered order.

Some displays may be ordered last, by using a "catchall" regex e.g.
```yaml
ORDER:
    - '!.*Monitor Maker ABC123.*$'
    - '!.*$'
    - 'DP-5'
```
Note that partial matches are not possible in this configuration, and the last displays must be exactly specified.

### SCALING

Enable scaling, overrides AUTO_SCALE and SCALE

```yaml
SCALING: false
```

### AUTO_SCALE

The default is to scale each display by DPI.

This may be disabled and scale 1 will be used, unless a `SCALE` has been specified.

```yaml
AUTO_SCALE: false
```

### SCALE

Auto scale may be overridden with custom scales for each display e.g.
```yaml
SCALE:
    - NAME_DESC: 'Monitor Maker ABC123'
      SCALE: 1.75
```

### MODE

*WARNING:* selecting some modes may result in an unusable (blank screen or powered off) monitor. Try this [workaround](../README.md#known-issues-with-workarounds) if you experience problems.

If the specified mode cannot be found or activated, `way-displays` will fall back to the preferred mode, then the highest available resolution / refresh.

Resolution with highest refresh:
```yaml
MODE:
    - NAME_DESC: HDMI-A-1
      WIDTH: 1920
      HEIGHT: 1080
```

Resolution and refresh:
```yaml
MODE:
    - NAME_DESC: HDMI-A-1
      WIDTH: 1920
      HEIGHT: 1080
      HZ: 60
```

When selecting a mode, `way-displays` will use the highest refresh that matches. There will usually be several refresh rates will match a specified number of Hz, differing only by a few mHz. These will be tried in descending order until a working one is found.

Maximum resolution and refresh:
```yaml
MODE:
    - NAME_DESC: HDMI-A-1
      MAX: TRUE
```

### TRANSFORM

Rotate or translate the display.
`90, `180`, `270`, `FLIPPED`, `FLIPPED-90`, `FLIPPED-180`, `FLIPPED-270`

e.g.
```yaml
TRANSFORM:
    - NAME_DESC: 'Monitor Maker ABC123'
      TRANSFORM: FLIPPED-90
```

### VRR_OFF

Adaptive sync is enabled by default. Disable it per display.

```yaml
VRR_OFF:
    - DP-2
    - '!.*my monitor.*'
```

### LAPTOP_DISPLAY_PREFIX

Laptop displays usually start with `eDP` e.g. `eDP-1`. This may be overridden if your laptop is different e.g.:
```yaml
LAPTOP_DISPLAY_PREFIX: 'eDPP'
```

### MAX_PREFERRED_REFRESH (deprecated)

Use `MODE`, specifying the preferred resolution.

### DISABLED

Disable the specified displays.

```yaml
DISABLED:
  - 'Monitor Maker ABC123'
  - 'HDMI-1'
```

## Command Line

Manages the server. The active configuration and display state may be inspected, and the configuration modified.

The active configuration can be written to disk, however any comments and formatting will be lost.

See `way-displays --help` and `man way-displays` for details.

```
Usage: way-displays [OPTIONS...] [COMMAND]
  Runs the server when no COMMAND specified.
OPTIONS
  -L, --l[og-threshold] <debug|info|warning|error>
  -c, --c[onfig]        <path>
  -y, --y[aml]          YAML client output, implies -L warning
COMMANDS
  -h, --h[elp]    show this message
  -v, --v[ersion] display version information
  -g, --g[et]     show the active settings
  -w, --w[rite]   write active to cfg.yaml
  -s, --s[et]     add or change
     ARRANGE_ALIGN <row|column> <top|middle|bottom|left|right>
     ORDER <name> ...
     SCALING <on|off>
     AUTO_SCALE <on|off>
     SCALE <name> <scale>
     MODE <name> MAX
     MODE <name> <width> <height> [<Hz>]
     TRANSFORM <name> <90|180|270|flipped|flipped-90|flipped-180|flipped-270>
     DISABLED <name>
     VRR_OFF <name>
  -d, --d[elete]  remove
     SCALE <name>
     MODE <name>
     TRANSFORM <name>
     DISABLED <name>
     VRR_OFF <name>
```

### Examples

Show the active configuration, commands and current display state
```sh
way-displays -g
```

Arrange left to right, aligned at the bottom
```sh
way-displays -s ARRANGE_ALIGN row bottom
```

Set the order for arrangement
```sh
way-displays -s ORDER HDMI-1 "monitor maker ABC model XYZ" eDP-1
```

Set a scale
```sh
way-displays -s SCALE "eDP-1" 3
```

Use 3840x2160@24Hz
```sh
way-displays -s MODE HDMI-A-1 3840 2160 24
```

Persist your changes to your `cfg.yaml`
```sh
way-displays -w
```

