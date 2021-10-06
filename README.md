# way-displays

Wayland successor to [xlayoutdisplay](https://github.com/alex-courtis/xlayoutdisplay)

## Goals

1. Sets preferred mode / highest at maximum refresh
1. Arrange left to right
1. Auto scales based on DPI
1. Reacts to hotplugged displays
1. Turns off laptop display when lid closed

## Usage

Run once after your compositor, such as [sway](https://swaywm.org/), has been started. `way-displays` will remain in the background responding to changes, such as plugging in a display, and will terminate once you exit the compositor.

`way-displays` will print messages to inform you of everything that is going.

### Example Usage Pattern: sway

## Configuration

The following options are available:
1. Define the left to right order of displays
1. Disable auto scaling and use a scale of 1
1. Set a custom scale for a monitor
1. Define a laptop display name prefix

