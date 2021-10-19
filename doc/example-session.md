## This session was run using the following configuration:

```yaml
ORDER:
    # I like my laptop on the left
    - 'Laptop Company 0x123A 456'
    - 'DP-2'
    - 'Monitor Maker ABC123'

SCALE:
    # this one is too far away; scale it up!
    - NAME_DESC: 'Monitor Maker ABC123'
      SCALE: 1.25

# my laptop is unusual
LAPTOP_DISPLAY_PREFIX: 'eDPP'
```

## Starting up:
```
way-displays version 1.0.2

Found configuration file: /home/me/.config/way-displays/cfg.yaml
  Auto scale: ON
  Laptop display prefix: 'eDPP'
  Order:
    Laptop Company 0x123A 456
    DP-2
    Monitor Maker ABC123
  Scale:
    Monitor Maker ABC123: 1.25

eDPP-1 Arrived:
  info:
    name:     'eDPP-1'
    desc:     'Laptop Company 0x123A 456 (eDPP-1)'
    width:    340mm
    height:   190mm
    dpi:      287.81 @ 3840x2160
  current:
    scale:    1.000
    position: 0,0
    mode:     3840x2160@60Hz (preferred)

eDPP-1 Changing:
  from:
    scale:    1.000
    position: 0,0
    mode:     3840x2160@60Hz (preferred)
  to:
    scale:    3.000

Changes successful

Monitoring lid device: /dev/input/event0
```

## Plugged in a monitor:
```
DP-3 Arrived:
  info:
    name:     'DP-3'
    desc:     'Monitor Maker ABC123 (DP-3 via HDMI)'
    width:    700mm
    height:   390mm
    dpi:      93.34 @ 2560x1440
  current:
    scale:    1.000
    position: 0,0
    mode:     2560x1440@144Hz (preferred)

DP-3 Changing:
  from:
    scale:    1.000
    position: 0,0
    mode:     2560x1440@144Hz (preferred)
  to:
    scale:    1.250
    position: 1280,0

Changes successful
```

## Closed the laptop lid:
```
eDPP-1 Changing:
  from:
    scale:    3.00
    position: 0,0
    mode:     3840x2160@60Hz (preferred)
    (lid closed)
  to:
    (disabled)

DP-3 Changing:
  from:
    scale:    1.25
    position: 1280,0
    mode:     2560x1440@144Hz (preferred)
  to:
    position: 0,0
Lid closed

eDPP-1 Changing:
  from:
    scale:    3.000
    position: 0,0
    mode:     3840x2160@60Hz (preferred)
    (lid closed)
  to:
    (disabled)

DP-3 Changing:
  from:
    scale:    1.250
    position: 1280,0
    mode:     2560x1440@144Hz (preferred)
  to:
    position: 0,0

Changes successful
```

## Opened the laptop lid:
```
Lid opened

eDPP-1 Changing:
  from:
    (disabled)
  to:
    scale:    3.000
    position: 0,0
    mode:     3840x2160@60Hz (preferred)
    (enabled)

DP-3 Changing:
  from:
    scale:    1.250
    position: 0,0
    mode:     2560x1440@144Hz (preferred)
  to:
    position: 1280,0

Changes successful
```

## Unplugged the monitor:
```
DP-3 Departed:
    name:     'DP-3'
    desc:     'Monitor Maker ABC123 (DP-3 via HDMI)'

No changes needed
```

