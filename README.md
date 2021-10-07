# way-displays Manages Your Wayland Displays

1. Sets preferred mode or highest at maximum refresh
1. Arranges left to right
1. Auto scales based on DPI: 96 is a scale of 1
1. Reacts when displays are plugged/unplugged
1. Reacts when laptop lid is closed/opened

Wayland successor to [xlayoutdisplay](https://github.com/alex-courtis/xlayoutdisplay)

# TODO requirements

# Usage

Run once after your wayland compositor has been started. `way-displays` will remain in the background, responding to changes, such as plugging in a display, and will terminate when you exit the compositor.

`way-displays` will print messages to inform you of everything that is going on.

## Example: [sway](https://swaywm.org/)

sway will start way-displays once on startup via the `exec` command. See `man 5 sway`.

Remove any `output` commands from your sway config file and add the following:
```
exec way-displays > /tmp/way-displays.${XDG_VTNR}.${USER}.log 2>&1
```

Look at `/tmp/way-displays.1.me.log` to see what has been going on.

# Configuration

TODO where

## Option: Order

The default left to right order is simply the order in which the displays are discovered.

Define this yourself e.g.
```yaml
ORDER_NAME_DESC:
    - 'DP-2'
    - 'Monitor Maker ABC123'
```

## Option: Auto Scaling

The default is to scale each display by DPI. This may be disabled:
```yaml
AUTO_SCALE: false
```

## Option: Custom Scales

Auto scale may be overridden by display e.g.:
```yaml
DISPLAY_SCALE:
    - NAME_DESC: 'Monitor Maker ABC123'
      SCALE: 1.25
```

## Option: Laptop Display Name Prefix

Laptop displays usually start with `eDP` e.g. `eDP-1`. This may be overridden if your laptop is different e.g.:
```yaml
LAPTOP_DISPLAY_PREFIX: 'eDERP'
```

## On Names and Descriptions

You can configure displays by name or description. You can find these by looking at the logs e.g.
```
DP-3 Arrived:
    name:     'DP-3'
    desc:     'Unknown Monitor Maker ABC123 (DP-3 via HDMI)'
```

It is recommended to use the description rather than the name, as the name may change over time and will definitely be different on different PCs.

The description does contain information about how it is connected, so strip that out. In the above example, you would use the description `Monitor Maker ABC123`.

# Installation

## TODO AUR

## Building From Source

Dependencies:
* gcc
* wayland
* wayland-protocols
* wlroots
* libinput
* yaml-cpp

Most will be available if you are running a wlroots based compositor like sway.

yaml-cpp will need to be installed via your distribution's package manager.

### Build

```
git clone git@github.com:alex-courtis/way-displays.git
cd way-displays
make
```

### Install / Uninstall

```
sudo make install
sudo make uninstall
```

# Contributing

Enhancements and bug fixes are very welcome: please raise an issue or fork this repo and submit a PR.

