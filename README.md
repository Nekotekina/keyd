# keyd++

Linux lacks a good key remapping solution. In order to achieve satisfactory
results a medley of tools need to be employed (e.g xcape, xmodmap) with the end
result often being tethered to a specified environment (X11). keyd attempts to
solve this problem by providing a flexible system wide daemon which remaps keys
using kernel level input primitives (evdev, uinput).

# Goals

  - Speed       (event loop that takes <<1ms for input event)
  - Simplicity  (a [config format](#sample-config) that is intuitive)
  - Consistency (modifiers that [play nicely with layers](docs/keyd.scdoc#L128) by default)
  - Modularity  (a UNIXy core extensible through the use of an [IPC](docs/keyd.scdoc#L391) mechanism)

# Features

keyd has several unique features many of which are traditionally only
found in custom keyboard firmware like [QMK](https://github.com/qmk/qmk_firmware)
as well as some which are unique to keyd.

Some of the more interesting ones include:

- Layers (with support for [hybrid modifiers](docs/keyd.scdoc#L128)).
- Key overloading (different behaviour on tap/hold).
- Keyboard specific configuration.
- Instantaneous remapping (no more flashing :)).
- A client-server model that facilitates scripting and display server agnostic application remapping. (Currently ships with support for X, sway, and gnome (wayland)).
- System wide config (works in a VT).
- First class support for modifier overloading.
- Unicode support.

keyd++ has specific features at the moment:

- Virtually unlimited sizes/counts (keyd has had many hardcoded limitations).
- Layer indicator with keyboard led of choice (keyd is somewhat broken).
- **Macro** can now do `type(Hello world)` without worrying about spaces.
- **Macro** can now execute `cmd(gnome-terminal)` and it should **just work**.
- Allow using `ctrl` as an alias for `control` (my personal whim).
- Wildcard for mice `m:` that excludes problematic abs ptr devices(`a:`).
- More flexible text parsing (in progress, eg. 'a+b' now equals 'b+a').
- Memory optimizations (sometimes only 1/5 of what keyd has had).
- Performance optimizations, eg. events from ungrabbed device are ignored.
- Some security improvement: privileged commands shall be in /etc/keyd/ conf.
- Commands coming from ex. `keyd-application-mapper` inherit uid+gid+environ.
- New commands for config reset control: push, pop, pop_all.
- Convenience and safety coming from C++20. Sorry for longer compilation.

### keyd is for people who:

 - Would like to experiment with custom [layers](https://docs.qmk.fm/feature_layers) (i.e custom shift keys)
   and oneshot modifiers.
 - Want to have multiple keyboards with different layouts on the same machine.
 - Want to be able to remap `C-1` without breaking modifier semantics.
 - Want a keyboard config format which is easy to grok.
 - Like tiny daemons that adhere to the Unix philosophy.
 - Want to put the control and escape keys where God intended.
 - Wish to be able to switch to a VT to debug something without breaking their keymap.

### What keyd isn't:

 - A tool for programming individual key up/down events.

# Dependencies

 - C++20 compiler starting from clang++-14 or g++-11
 - Linux kernel headers (already present on most systems)

## Optional

 - python      (for application specific remapping)
 - python-xlib (only for X support)
 - dbus-python (only for KDE support)

# Installation

*Note:* master serves as the development branch, things may occasionally break.

## From Source

```bash
# Install dependencies (if necessary)
sudo apt install build-essentials git
# Clone with git clone (.) or download sources manually to keyd directory
cd keyd
# Specify your favourite compiler (optional)
export CXX=clang++-18
# First time install
make && sudo make install && sudo systemctl enable --now keyd
# Second time install (update, contains **example** flags for statically linking libstdc++)
CXX=clang++-18 CXXFLAGS='-static-libgcc -static-libstdc++' make && sudo make install && sudo systemctl daemon-reload && sudo systemctl restart keyd
```

# Quickstart

1. Install and start keyd (e.g `sudo systemctl enable keyd --now`)

2. Put the following in `/etc/keyd/default.conf`:

```
[ids]

*

[main]

# Maps capslock to escape when pressed and control when held.
capslock = overload(control, esc)

# Remaps the escape key to capslock
esc = capslock
```

Key names can be obtained by using the `keyd monitor` command. Note that while keyd is running, the output of this
command will correspond to keyd's output. The original input events can be seen by first stopping keyd and then
running the command. See the man page for more details.

3. Run `sudo keyd reload` to reload the config set.

4. See the man page (`man keyd`) for a more comprehensive description.

Config errors will appear in the log output and can be accessed in the usual
way using your system's service manager (e.g `sudo journalctl -eu keyd`).

*Note*: It is possible to render your machine unusable with a bad config file.
Should you find yourself in this position, the special key sequence
`backspace+escape+enter` should cause keyd to terminate.

Some mice (e.g the Logitech MX Master) are capable of emitting keys and
are consequently matched by the wildcard id. It may be necessary to
explicitly blacklist these.

## Application Specific Remapping (experimental)

- Add yourself to the keyd group:

	`usermod -aG keyd <user>`

- Populate `~/.config/keyd/app.conf`:

E.G

	[alacritty]

	alt.] = macro(C-g n)
	alt.[ = macro(C-g p)

	[chromium]

	alt.[ = C-S-tab
	alt.] = macro(C-tab)

- Run:

	`keyd-application-mapper`

You will probably want to put `keyd-application-mapper -d` somewhere in your
display server initialization logic (e.g ~/.xinitrc) unless you are running Gnome.

See the man page for more details.

## SBC support

Experimental support for single board computers (SBCs) via usb-gadget
has been added courtesy of Giorgi Chavchanidze.

See [usb-gadget.md](src/vkbd/usb-gadget.md) for details.

## Packages

Third party packages for the some distributions also exist. If you wish to add
yours to the list please file a PR. These are kindly maintained by community
members, no personal responsibility is taken for them.

# Sample Config

	[ids]

	k:*

	[main]

	leftshift = oneshot(shift)
	capslock = overload(symbols, esc)

	[symbols]

	d = ~
	f = /
	...

# Example config

Many users will probably not be interested in taking full advantage of keyd.
For those who seek simple quality of life improvements I can recommend the
following config:

	[ids]

	k:*

	[main]

	shift = oneshot(shift)
	meta = oneshot(meta)
	control = oneshot(control)

	leftalt = oneshot(alt)
	rightalt = oneshot(altgr)

	capslock = overload(control, esc)
	insert = S-insert

This overloads the capslock key to function as both escape (when tapped) and
control (when held) and remaps all modifiers to 'oneshot' keys. Thus to produce
the letter A you can now simply tap shift and then a instead of having to hold
it. Finally it remaps insert to S-insert (paste on X11).

# Example keyd-application-mapper configuration

	[org-gnome-nautilus]
	meta.f = macro(C-c 50ms cmd(xsel -ocb | xargs firefox))

This command will open (hopefully) selected file in Firefox.
Be aware that this command copies the file into the clipboard.
This assumes GNOME with Nautilus file browser and X or Xwayland.
