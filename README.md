# way-displays: Auto Manage Your Wayland Displays

<img align="right" width="427" height="189" title="credit: Stephen Barratt" src="doc/layouts.png?raw=true">

1. Set resolution/refresh: preferred, highest or custom

1. Enable VRR / adaptive sync

1. Arrange in a row or a column

1. Auto scale based on DPI: 96 is a scale of 1

1. Update when displays plugged/unplugged

1. Update when laptop lid closed/opened

Works out of the box: no configuration required.

Wayland successor to [xlayoutdisplay](https://github.com/alex-courtis/xlayoutdisplay), inspired by [kanshi](https://sr.ht/~emersion/kanshi/).

See wiki for [Configuration](https://github.com/alex-courtis/way-displays/wiki/Configuration), [Recipes](https://github.com/alex-courtis/way-displays/wiki/Recipes), [Troubleshooting](https://github.com/alex-courtis/way-displays/wiki/Troubleshooting) and more.

## Requirements

way-displays is known to work on the [sway](https://swaywm.org/), [river](https://github.com/riverwm/river) and [Hpyrland](https://hyprland.org/) compositors. It may work on any wlroots compositor that supports the WLR Output Management protocol.

The user must be a member of the `input` group.

## Quick Start

```
mkdir -p ~/.config/way-displays
cp /etc/way-displays/cfg.yaml ~/.config/way-displays/cfg.yaml
```

See [Configuration](https://github.com/alex-courtis/way-displays/wiki/Configuration)

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

### Hyprland

Create a launcher: `${HOME}/.config/hypr/start-way-displays.sh`
```sh
#!/bin/sh

sleep 1 # give Hyprland a moment to set its defaults

way-displays > "/tmp/way-displays.${XDG_VTNR}.${USER}.log" 2>&1
```

Make it executable:
```sh
chmod 755 ${HOME}/.config/hypr/start-way-displays.sh
```

Add the following to your `hyprland.conf`:
```
exec-once = ${HOME}/.config/hypr/start-way-displays.sh
```

### Configure

Restart the compositor and run `way-displays -g` or look at `/tmp/way-displays.1.me.log`.

Tweak [cfg.yaml](https://github.com/alex-courtis/way-displays/wiki/Configuration#cfgyaml) to your liking and save it. Changes will be immediately applied.

Alternatively, use the [command line](https://github.com/alex-courtis/way-displays/wiki/Configuration#command-line) to make your changes then persist them with `way-displays -w`.

You might want to `tail -f /tmp/way-displays.1.me.log` whilst you are tweaking.

## Usage

See [Configuration](https://github.com/alex-courtis/way-displays/wiki/Configuration) for details on `cfg.yaml` and the command line.

Start the `way-displays` server by running once with no arguments after your wayland compositor has been started.

It will remain in the background, responding to changes, such as plugging in a display, and will terminate when you exit the compositor.

It will print messages to inform you of everything that is going on.

You can interact with the server via the [command line](https://github.com/alex-courtis/way-displays/wiki/Configuration#command-line)

The server responds to [IPC](https://github.com/alex-courtis/way-displays/wiki/IPC) requests to fetch and mutate state.

## Installation

### Package Manager

[![Packaging status](https://repology.org/badge/vertical-allrepos/way-displays.svg)](https://repology.org/project/way-displays/versions)

### From Source

[![CI](https://github.com/alex-courtis/way-displays/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/alex-courtis/way-displays/actions/workflows/ci.yml?query=branch%3Amaster)

See [CONTRIBUTING](CONTRIBUTING.md)

#### Install / Uninstall

```
sudo make install
sudo make uninstall
```

## Questions, Suggestions And Contributions

Please create a [github issue](https://github.com/alex-courtis/way-displays/issues) and attach the log `/tmp/way-displays.1.me.log`

[Contributions](CONTRIBUTING.md) are most gratefully received, see [Milestones](https://github.com/alex-courtis/way-displays/milestones) for prioritised issues.
