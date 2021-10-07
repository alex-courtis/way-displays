This session was run using the following configuration:
```yaml
ORDER:
    # I like my laptop on the left
    - 'Laptop Company 0x123A 456'
    - 'Monitor Maker ABC123'
    - 'DP-2'

SCALE:
    # this one is too far away; scale it up!
    - NAME_DESC: 'Monitor Maker ABC123'
      SCALE: 1.25

# my laptop is weird
LAPTOP_DISPLAY_PREFIX: 'eDERP'
```

Starting up:
```
Configuration file: /home/me/.config/way-displays/cfg.yaml
  Auto scale: ON
  Laptop display prefix: 'eDERP'
  Order:
    Laptop Company 0x123A 456
    Monitor Maker ABC123
    DP-2
  Scale:
    Monitor Maker ABC123: 1.25

Monitoring lid device: /dev/input/event0

eDERP-1 Arrived:
    name:     'eDERP-1'
    desc:     'Laptop Company 0x123A 456 (eDERP-1)'
    width:    340mm
    height:   190mm
    scale:    1.00
    position: 0,0
    mode:     3840x2160@60Hz (preferred)

eDERP-1 Changing:
  from:
    scale:    1.00
    position: 0,0
    mode:     3840x2160@60Hz (preferred)
  to:
    scale:    3.00

Changes successful
```

Plugged in a monitor:
```
DP-3 Arrived:
    name:     'DP-3'
    desc:     'Unknown Monitor Maker ABC123 (DP-3 via HDMI)'
    width:    700mm
    height:   390mm
    scale:    1.00
    position: 0,0
    mode:     2560x1440@144Hz (preferred)

DP-3 Changing:
  from:
    scale:    1.00
    position: 1280,0
    mode:     2560x1440@144Hz (preferred)
  to:
    scale:    1.25
    position: 1280,0

Changes successful
```

Closed the laptop lid:
```
eDERP-1 Changing:
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

Changes successful
```

Opened the laptop lid:
```
eDERP-1 Changing:
  from:
    (disabled)
  to:
    scale:    3.00
    position: 0,0
    mode:     3840x2160@60Hz (preferred)
    (enabled)
DP-3 Changing:
  from:
    scale:    1.25
    position: 0,0
    mode:     2560x1440@144Hz (preferred)
  to:
    position: 1280,0

Changes successful
```

Unplugged the monitor:
```
'DP-3' Departed:
    name:     'DP-3'
    desc:     'Unknown Monitor Maker ABC123 (DP-3 via HDMI)'

No changes needed
```

