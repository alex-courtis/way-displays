# way-displays: Auto Manage Your Wayland Displays

<img align="right" width="427" height="189" title="credit: Stephen Barratt" src="doc/layouts.png?raw=true">

1. Set mode: user, preferred or highest

1. Arrange in a row or a column

1. Auto scale based on DPI: 96 is a scale of 1

1. Update when displays plugged/unplugged

1. Update when laptop lid closed/opened

Works out of the box: no configuration required.

Wayland successor to [xlayoutdisplay](https://github.com/alex-courtis/xlayoutdisplay), inspired by [kanshi](https://sr.ht/~emersion/kanshi/).

<!-- gh-md-toc --no-backup --hide-footer README.md -->
<!--ts-->
   * [Requirements](#requirements)
   * [Quick Start](#quick-start)
   * [Usage](#usage)
   * [Installation](#installation)
      * [Package Manager](#package-manager)
      * [From Source](#from-source)
   * [Known Issues with Workarounds](#known-issues-with-workarounds)
   * [What Is Preferred Mode?](#what-is-preferred-mode)
   * [On Scale And Blurring](#on-scale-and-blurring)
   * [Questions, Suggestions And Ideas](#questions-suggestions-and-ideas)
   * [Help Wanted - GUI Configurator](#help-wanted---gui-configurator)
<!--te-->

## Requirements

way-displays is known to work on the [sway](https://swaywm.org/) and [river](https://github.com/riverwm/river) compositors. It may work on any wlroots compositor that supports the WLR Output Management protocol.

The user must be a member of the `input` group.

## Quick Start

[Configuration](doc/configuration.md)

```
mkdir -p ~/.config/way-displays
cp /etc/way-displays/cfg.yaml ~/.config/way-displays/cfg.yaml
```

### Sway

Remove any `output` commands from your sway config file and add the following:
```
exec way-displays > /tmp/way-displays.${XDG_VTNR}.${USER}.log 2>&1
```

### River

Add the following to your `init`:
```
way-displays > /tmp/way-displays.${XDG_VTNR}.${USER}.log 2>&1 &
```

### Configure

Restart the compositor and look at `/tmp/way-displays.1.me.log` to see what has been going on. You can tail it whilst you customise.

[Tweak cfg.yaml](doc/configuration.md#cfgyaml) to your liking and save it. Changes will be immediately applied.

Alternatively, use the [command line](doc/configuration.md#command-line) to make your changes then persist them with `way-displays -w`.

## Usage

See [configuration](doc/configuration.md) for details on `cfg.yaml` and the command line.

Start the `way-displays` server by running once with no arguments after your wayland compositor has been started.

It will remain in the background, responding to changes, such as plugging in a display, and will terminate when you exit the compositor.

It will print messages to inform you of everything that is going on.

You can interact with the server via the [command line](doc/configuration.md#command-line).

## Installation

### Package Manager

[![Packaging status](https://repology.org/badge/vertical-allrepos/way-displays.svg)](https://repology.org/project/way-displays/versions)

### From Source

#### Dependencies
* GNU make
* gcc
* wayland
* wayland-protocols
* wlroots
* libinput
* yaml-cpp

Most will be available if you are running a wlroots based compositor like sway.

yaml-cpp will need to be installed via your distribution's package manager.

Set `CC=mycompiler` and `CXX=mycompiler++` if you don't like gcc.

#### Build

```
git clone git@github.com:alex-courtis/way-displays.git
cd way-displays
make
```

#### Install / Uninstall

```
sudo make install
sudo make uninstall
```

## Known Issues with Workarounds

### Laptop Lid Not Detected - Permission Denied

```
W [10:09:44.542] WARNING: open '/dev/input/event0' failed 13: 'Permission denied'
```

User must be in the `input` group to monitor libinput events.

### Laptop Lid Not Closed At Startup

Fixed in [libinput 1.21.0](https://gitlab.freedesktop.org/libinput/libinput/-/releases/1.21.0).

### Unusable Displays Following MODE

One or many displays may be rendered unusable after setting a `MODE`. This has occurred when a higher resolution/refresh than the preferred has been selected, particularly when using a HDMI cable.

It may be possible to work around this by setting `WLR_DRM_NO_MODIFIERS=1`. See [wlroots documentation](https://gitlab.freedesktop.org/wlroots/wlroots/-/blob/master/docs/env_vars.md) for details.

You can set it when directly starting sway e.g.
```shell
WLR_DRM_NO_MODIFIERS=1 sway ...
```

If you use a display manager, you will need to export it from your non-login shell environment e.g. `.zshenv`.
```shell
export WLR_DRM_NO_MODIFIERS=1
```

### Scaling Breaks X11 Games

When a display is scaled (X11) linux games will render at the display's scaled resolution, rather than the monitor's native resolution. There is [work underway](https://gitlab.freedesktop.org/wlroots/wlroots/-/issues/2125) to fix this.

In the meantime, auto scale may be temporarily disabled via `way-displays -s AUTO_SCALE off`.

Any explicily specified `SCALE` values will override `AUTO_SCALE: false`, so you would need to temporarily remove those via `way-displays -d SCALE "my monitor"`

## What Is Preferred Mode?

Displays advertise their available modes when plugged in. Some displays specify a mode as "preferred".

The preferred mode is usually the highest resolution/refresh available and it's a good default. You shouldn't need to tweak this.

In some cases the preferred mode is a horrid "compatibility" mode e.g. `1024x768@60Hz`. You could fix this by setting `MODE` to `MAX` for that display.

## On Scale And Blurring

When using a display scale that is not a whole number, the result will not be a pixel perfect rendition of the unscaled content. There are no fractional pixels so there will be rounding and thus some blurring.

To ameliorate this, we always round our scale to a multiple of one eighth. This results in a nice round binary number, which minimises some of the rounding and results in a smoother image. If you're interested, our rounded scale is a [wl_fixed_t](https://wayland.freedesktop.org/docs/html/apb.html).

## Questions, Suggestions And Ideas

Please create a [github issue](https://github.com/alex-courtis/way-displays/issues) and attach the log `/tmp/way-displays.1.me.log`

[Contributions](CONTRIBUTING.md) are always welcome.

## Help Wanted - GUI Configurator

way-displays can currently be configured via the configuration file and CLI.

A GUI client would greatly enhance usability.

The client could use the same IPC socket and YAML streaming protocol as the CLI.

