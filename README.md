# way-displays: Manage Your Wayland Displays

1. Sets preferred mode or highest at maximum refresh
1. Arranges left to right
1. Auto scales based on DPI: 96 is a scale of 1
1. Reacts when displays are plugged/unplugged
1. Reacts when laptop lid is closed/opened

See an [example session](doc/example-session.md) for more details.

Wayland successor to [xlayoutdisplay](https://github.com/alex-courtis/xlayoutdisplay)

Inspired by [kanshi](https://sr.ht/~emersion/kanshi/)

# Requirements

The wayland compositor must support the WLR (wayland roots) Output Management protocol.

Such compositors include:
* [sway](https://swaywm.org/)
* [hikari](https://hikari.acmelabs.space)
* [Way Cooler](http://way-cooler.org/)
* [Wayfire](https://github.com/WayfireWM/wayfire)

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

The following are used, in order: `$XDG_CONFIG_HOME/way-displays/cfg.yaml`, `$HOME/.config/way-displays/cfg.yaml`, `/usr/local/etc/way-displays/cfg.yaml`, `/etc/way-displays/cfg.yaml`

See [default configuration](cfg.yaml) at `/etc/way-displays/cfg.yaml`.

To get started:
```
mkdir ~/.config/way-displays
cp /etc/way-displays/cfg.yaml ~/.config/way-displays/cfg.yaml
```

`cfg.yaml` will be monitored for changes, which will be immediately applied.

## Option: Order

The default left to right order is simply the order in which the displays are discovered.

Define this own e.g.:
```yaml
ORDER:
    - 'DP-2'
    - 'Monitor Maker ABC123'
```

## Option: Auto Scaling

The default is to scale each display by DPI.

This may be disabled and scale 1 will be used, unless a `SCALE` has been specified.

```yaml
AUTO_SCALE: false
```

## Option: Custom Scales

Auto scale may be overridden for each display e.g.
```yaml
SCALE:
    - NAME_DESC: 'Monitor Maker ABC123'
      SCALE: 1.75
```

## Option: Laptop Display Name Prefix

Laptop displays usually start with `eDP` e.g. `eDP-1`. This may be overridden if your laptop is different e.g.:
```yaml
LAPTOP_DISPLAY_PREFIX: 'eDPP'
```

## On Names and Descriptions

You can configure displays by name or description. You can find these by looking at the logs e.g.
```
DP-3 Arrived:
    name:     'DP-3'
    desc:     'Unknown Monitor Maker ABC123 (DP-3 via HDMI)'
```

It is recommended to use the description rather than the name, as the name may change over time and will most likely be different on different PCs.

The description does contain information about how it is connected, so strip that out. In the above example, you would use the description `Monitor Maker ABC123`.

# Installation

## AUR

[way-displays](https://aur.archlinux.org/packages/way-displays/)

Install with your favourite package manager e.g. `pacaur -S way-displays`

## Build From Source

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

# On Scale And Blurring

When using a display scale that is not a whole number, the result will not be a pixel perfect rendition of the unscaled content. There are no fractional pixels so there will be rounding and thus some blurring.

To ameliorate this, we always round our scale to a multiple of one eighth. This results in a nice round binary number, which minimises some of the rounding and results in a smoother image. If you're interested, our rounded scale is a [wl_fixed_t](https://wayland.freedesktop.org/docs/html/apb.html).

# Help, Questions, Suggestions And Ideas

Please create a [github issue](https://github.com/alex-courtis/way-displays/issues).

# Contributing

Enhancements and bug fixes are very welcome: please raise an issue or fork this repo and submit a PR.

