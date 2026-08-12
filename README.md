# New Release 2026-09-17: 2.0.0

See [Version 2.0.0 Changes](https://github.com/alex-courtis/way-displays/wiki/Version-2.0.0-Changes)

# way-displays: Auto Manage Your Wayland Displays

<img align="right" width="427" height="189" title="credit: Stephen Barratt" src="doc/layouts.png?raw=true">

1. Set resolution/refresh: preferred, highest or custom

1. Enable VRR / adaptive sync

1. Arrange in a row or a column

1. Auto scale based on DPI: 96 is a scale of 1

1. Update when displays plugged/unplugged with optionally defined conditions

1. Update when laptop lid closed/opened

Works out of the box: no configuration required.

Communicates verbosely via logs/stdout and notifications.

Wayland successor to [xlayoutdisplay](https://github.com/alex-courtis/xlayoutdisplay), inspired by [kanshi](https://sr.ht/~emersion/kanshi/).

See wiki for [Configuration](https://github.com/alex-courtis/way-displays/wiki/Configuration), [Recipes](https://github.com/alex-courtis/way-displays/wiki/Recipes), [Troubleshooting](https://github.com/alex-courtis/way-displays/wiki/Troubleshooting) and more.

## Requirements

<!-- copied from doc/way-displays.1.md -->
A wlroots based compositor that supports the WLR Output Management protocol version 4+.

way-displays is blessed for the [sway](https://swaywm.org/), [river](https://codeberg.org/river/river) and [river-classic](https://codeberg.org/river/river-classic) compositors. It may work on others; please tell me of your experiences!

[Hpyrland](https://hyprland.org/) already provides all the features of `way-displays`. It may function, however it is explicitly not supported and you will likely experience problems. Please do not raise issues.

way-displays must be run as a daemon, a background server process. It will respond to your configuration changes as well as state changes such as plugging in a monitor or closing the lid.

User should be a member of the **`input`** UNIX group for querying laptop lid state.
<!-- end copy -->

## Documentation

[`man way-displays`](doc/way-displays.1.md) for server and CLI details.

[`man 5 way-displays`](doc/way-displays.5.md) for `cfg.yaml` config file reference.

[wiki: Recipes](https://github.com/alex-courtis/way-displays/wiki/Recipes)

[wiki: IPC](https://github.com/alex-courtis/way-displays/wiki/IPC)

## Quick Start

<!-- copied from doc/way-displays.1.md -->
Start the way-displays server:

**Sway**  
Remove any `output` commands from your sway config file and add the following:

``` sh
exec way-displays > /tmp/way-displays.${XDG_VTNR}.${USER}.log 2>&1
```

**River**  
Add the following to your [River Window Manager](https://codeberg.org/river/wiki/src/branch/main/pages/wm-list.md)’s init script or similar:

``` sh
exec way-displays > /tmp/way-displays.${XDG_VTNR}.${USER}.log 2>&1
```

**River Classic**  
Add the following to your `${XDG_CONFIG_HOME}/river/init`:

``` sh
way-displays > /tmp/way-displays.${XDG_VTNR}.${USER}.log 2>&1 &
```

**Configure**

Restart the compositor and run **`way-displays -g`** or look at **`/tmp/way-displays.${XDG_VTNR}.${USER}.log`**

**`way-displays`** will communicate verbosely via the logs - you might want to **`tail -f /tmp/way-displays.${XDG_VTNR}.${USER}.log`** whilst you are tweaking.

Tweak [**`cfg.yaml`**](/examples/cfg.yaml) to your liking and save it, see [**`man 5 way-displays`**](/doc/way-displays.5.md). Changes will be immediately applied.

Alternatively, use the CLI to make your changes then persist them with **`way-displays -w`**
<!-- end copy -->

## Installation

### Package Manager

[![Packaging status](https://repology.org/badge/vertical-allrepos/way-displays.svg)](https://repology.org/project/way-displays/versions)

### From Source

master is unstable, a release tag is recommended for a stable experience.

[![CI](https://github.com/alex-courtis/way-displays/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/alex-courtis/way-displays/actions/workflows/ci.yml?query=branch%3Amaster)

See [CONTRIBUTING](CONTRIBUTING.md) for a list of the dependencies you'll need.

#### Install / Uninstall

```
sudo make install
sudo make uninstall
```

## Issues

Please [collect debug logs](https://github.com/alex-courtis/way-displays/wiki/Troubleshooting#logs-tell-you-everything), create a [github issue](https://github.com/alex-courtis/way-displays/issues) and attach your log.

## Questions, Ideas And Contributions

Please raise a [github issue](https://github.com/alex-courtis/way-displays/issues)

[Contributions](doc/CONTRIBUTING.md) are most gratefully received, see [Milestones](https://github.com/alex-courtis/way-displays/milestones) for prioritised issues.
