# way-displays: Auto Manage Your Wayland Displays

1. Sets preferred mode or highest at maximum refresh
1. Arranges in a row or a column
1. Auto scales based on DPI: 96 is a scale of 1
1. Reacts when displays are plugged/unplugged
1. Reacts when laptop lid is closed/opened

Works out of the box: no configuration required.

Command line client to inspect, modify and persist changes to the active configuration.

Wayland successor to [xlayoutdisplay](https://github.com/alex-courtis/xlayoutdisplay), inspired by [kanshi](https://sr.ht/~emersion/kanshi/).

<details><summary>Arrangements</summary><br>

![layouts](doc/layouts.png)

</details>

See an [example session](doc/example-session.md) for full details.

# Requirements

The wayland compositor must support the WLR (wayland roots) Output Management protocol.

<details><summary>Popular WLR Compositors</summary><br>

* [sway](https://swaywm.org/)
* [hikari](https://hikari.acmelabs.space)
* [Way Cooler](http://way-cooler.org/)
* [Wayfire](https://github.com/WayfireWM/wayfire)
</details>

# Usage

Run once after your wayland compositor has been started. `way-displays` will remain in the background, responding to changes, such as plugging in a display, and will terminate when you exit the compositor.

`way-displays` will print messages to inform you of everything that is going on.

<details><summary>sway config</summary><br>

[sway](https://swaywm.org/) will start way-displays once on startup via the `exec` command. See `man 5 sway`.

Remove any `output` commands from your sway config file and add the following:
```
exec way-displays > /tmp/way-displays.${XDG_VTNR}.${USER}.log 2>&1
```

Look at `/tmp/way-displays.1.me.log` to see what has been going on.

</details>

# Configuration

See the [default cfg.yaml](cfg.yaml) installed at `/etc/way-displays/cfg.yaml`.

Quick start:
```
mkdir ~/.config/way-displays
cp /etc/way-displays/cfg.yaml ~/.config/way-displays/cfg.yaml
```

`cfg.yaml` will be monitored for changes, which will be immediately applied.

<details><summary>File Locations</summary><br>

The following are used, in order:
* `$XDG_CONFIG_HOME/way-displays/cfg.yaml`
* `$HOME/.config/way-displays/cfg.yaml`
* `/usr/local/etc/way-displays/cfg.yaml`
* `/etc/way-displays/cfg.yaml`

</details>

<details><summary>ARRANGE and ALIGN</summary><br>

The default is to arrange in a row, aligned at the top of the displays. This is very configurable:

![layouts](doc/layouts.png)

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

</details>

<details><summary>ORDER</summary><br>

The default `ROW` (left to right) or `COLUMN` (top to bottom) `ORDER` is simply the order in which the displays are discovered.

Define your own e.g.:
```yaml
ORDER:
    - 'DP-2'
    - 'Monitor Maker ABC123'
```

</details>

<details><summary>AUTO_SCALE</summary><br>

The default is to scale each display by DPI.

This may be disabled and scale 1 will be used, unless a `SCALE` has been specified.

```yaml
AUTO_SCALE: false
```

</details>

<details><summary>SCALE</summary><br>

Auto scale may be overridden with custom scales for each display e.g.
```yaml
SCALE:
    - NAME_DESC: 'Monitor Maker ABC123'
      SCALE: 1.75
```

</details>

<details><summary>LAPTOP_DISPLAY_PREFIX</summary><br>

Laptop displays usually start with `eDP` e.g. `eDP-1`. This may be overridden if your laptop is different e.g.:
```yaml
LAPTOP_DISPLAY_PREFIX: 'eDPP'
```

</details>

<details><summary>MAX_PREFERRED_REFRESH</summary><br>

For the specified displays, use the maximum avalable refresh rate for resolution of the preferred mode.

e.g. when preferred mode is `1920x1080@60Hz`, use `1920x1080@165Hz`

Warning: this may result in an unusable display.

```yaml
MAX_PREFERRED_REFRESH:
  - 'Monitor Maker ABC123'
  - 'HDMI-1'
```

</details>

<details><summary>DISABLED</summary><br>

Disable the specified displays.

```yaml
DISABLED:
  - 'Monitor Maker ABC123'
  - 'HDMI-1'
```

</details>

<details><summary>On Names and Descriptions</summary><br>
You can configure displays by name or description. You can find these by looking at the logs e.g.

```
DP-3 Arrived:
    name:     'DP-3'
    desc:     'Unknown Monitor Maker ABC123 (DP-3 via HDMI)'
```

It is recommended to use the description rather than the name, as the name may change over time and will most likely be different on different PCs.

The description does contain information about how it is connected, so strip that out. In the above example, you would use the description `Monitor Maker ABC123`.

The name should be at least 3 characters long, to avoid any unwanted extra matches.

</details>

# Command Line Configuration

Manages the server. The active configuration and display state may be inspected, and the configuration modified.

The active configuration can be written to disk, however any comments and formatting will be lost.

See `way-displays --help` and `man way-displays` for details.

<details><summary>Examples</summary><br>

Show current configuration and display state: `way-displays -g`

Arrange left to right, aligned at the bottom: `way-displays -s ARRANGE_ALIGN row bottom`

Set the order for arrangement: `way-displays -s ORDER HDMI-1 "monitor maker ABC model XYZ" eDP-1`

Set a scale: `way-displays -s SCALE "eDP-1" 3`

Persist your changes to your cfg.yaml: `way-displays -w`

</details>

# Installation

## Package Manager

[![Packaging status](https://repology.org/badge/vertical-allrepos/way-displays.svg)](https://repology.org/project/way-displays/versions)

## From Source

<details><summary>Build</summary>

### Dependencies
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
</details>

# Known Issues with Workarounds

<details><summary>Laptop Lid Not Detected - Permission Denied</summary><br>

```
W [10:09:44.542] WARNING: open '/dev/input/event0' failed 13: 'Permission denied'
```

User must be in the `input` group to monitor libinput events.
</details>

<details><summary>Laptop Lid Not Closed At Startup</summary><br>

libinput only reports lid state at startup for _some_ lids. We can direct libinput to always report for our lid. See [Installing temporary local device quirks](https://wayland.freedesktop.org/libinput/doc/latest/device-quirks.html#device-quirks-local) for reference.

### 0 - Test Whether libinput Reports Your Lid
Note your lid's event device at way-displays startup e.g.
```
I [11:34:05] Monitoring lid device: /dev/input/event1
```

Run `libinput quirks list /dev/input/eventX`. If you don't see `AttrLidSwitchReliability=reliable`, libinput won't report the startup state.

### 1 - Determine Lid Switch's DMI
```
libinput record /dev/input/eventX | grep ^dmi
^C
```
Example dmi for ct31 switch:
```
Recording to 'stdout'.
dmi: "dmi:bvnLENOVO:bvrN2WET25W(1.15):bd12/07/2020:br1.15:efr1.9:svnLENOVO:pn20UBCTO1WW:pvrThinkPadX1YogaGen5:rvnLENOVO:rn20UBCTO1WW:rvrSDK0J40709WIN:cvnLENOVO:ct31:cvrNone:skuLENOVO_MT_20UB_BU_Think_FM_ThinkPadX1YogaGen5:"
```

### 2 - Create `/etc/libinput/local-overrides.quirks`:
```
[Lid Switch Ct31]
MatchName=*Lid Switch*
MatchDMIModalias=dmi:*:ct31:*
AttrLidSwitchReliability=reliable
```

You can put the entire dmi string in `MatchDMIModalias` or just the ctXX bit.

### 3 - Test libinput And way-displays
`libinput quirks list /dev/input/eventX`. You should see `AttrLidSwitchReliability=reliable`.

Close the lid and start way-displays. You should see:
```
I [11:34:05] Monitoring lid device: /dev/input/event1
I [11:34:05]
I [11:34:05] Lid closed
```

</details>

<details><summary>Scaling Breaks X11 Games</summary><br>

When a display is scaled (X11) linux games will render at the display's scaled resolution, rather than the monitor's native resolution. There is [work underway](https://gitlab.freedesktop.org/wlroots/wlroots/-/issues/2125) to fix this.

In the meantime, auto scale may be temporarily disabled via `way-displays -s AUTO_SCALE off`.

Any explicily specified `SCALE` values will override `AUTO_SCALE: false`, so you would need to temporarily remove those via `way-displays -d SCALE "my monitor"`
</details>

# On Scale And Blurring

When using a display scale that is not a whole number, the result will not be a pixel perfect rendition of the unscaled content. There are no fractional pixels so there will be rounding and thus some blurring.

To ameliorate this, we always round our scale to a multiple of one eighth. This results in a nice round binary number, which minimises some of the rounding and results in a smoother image. If you're interested, our rounded scale is a [wl_fixed_t](https://wayland.freedesktop.org/docs/html/apb.html).

# Help, Questions, Suggestions And Ideas

Please create a [github issue](https://github.com/alex-courtis/way-displays/issues) and attach the log `/tmp/way-displays.1.me.log`.

# Developing

Enhancements and bug fixes are very welcome: please raise a [github issue](https://github.com/alex-courtis/way-displays/issues) or fork this repo and submit a PR.

## Author

Alexander Courtis

## Contributors

Stephen Barratt
