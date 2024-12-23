# v2.5.0

 - Allow 64 character layer names
 - Introduce a new device id format to distinguish between devices with identical product/vendor id pairs (backward compatible)
 - Add KDE/plasma5 support to `keyd-application-mapper`
 - Gnome v45/v46 application remapping support
 - Increases the maximum number of descriptors to allow for more advanced configs
 - Add `setlayout()` to facilitate dynamically set layouts
 - `toggle()` now activates the layer on key down instead of key up
 - Improve support for exotic keys
 - Various minor bug fixes

# v2.4.3

## Summary (non exhaustive)

- Introduces a new layout type
- Improves application based remapping suport for wayland/X
- Adds `swap` support for toggled layers
- Adds support for chording
- Numerous bugfixes and stability improvements

## New Actions:

 - togglem (#270)
 - clear() (#253)
 - overloadt (#309)
 - overloadt2

## New Commands:

 - listen command (#294, #217)
 - reload
 - do
 - input

## New timeout and modifier knobs:

 - disable_modifier_guard (#257)
 - oneshot_timeout option
 - overload_tap_timeout
 - macro_sequence_timeout (#255)

See the manpage for details.


# v2.4.2

 - Add include directive to the config syntax
 - Ship includable common layouts
 - Allow comments in the ids section (#245)
 - Create virtual pointer on initialization (#249)
 - Misc bug fixes

# v2.4.1

 - Route button presses through the virtual keyboard (#162)
 - Improve mouse support
 - Fix VT repeat bug
 - Allow overload to accept an arbitrary action (#199)
 - Add support for full key serquences to swap()
 - Misc bugfixes

# v2.4.0

 - Fix macro timeouts
 - Allow timeouts to be used in conjunction with + (#181)
 - Add macro2() to allow for per-macro timeout values (#176)
 - Add command() to allow for command execution
 - Add [global] section
 - Improve unicode support
 - Add support for older kernels (#163)
 - Clear oneshot on click
 - Various bugfixes and enhancements

# v2.3.1-rc

 - Add unicode support
 - Add noop
 - Fix keyd-application-mapper hotswapping

# v2.3.0-rc

This is a **major release** which breaks **backward compatibility** with
non trivial configs. It is best to reread the man page. Minimal
breaking changes are expected moving forward.

 - Introduce composite layers
 - Add timeout()
 - Simplify layer model
 - Layer entries are now affected by active modifiers (current layer modifiers excepted)
 - Eliminate layer types
 - Eliminate -d

See [DESIGN.md](DESIGN.md) for a more thorough description of changes.

# v2.2.7-beta

 - Fix support for symlinked config files (#148)
 - Improve out of the box handling of alt and meta overloading (#128)
 - Add unicode support to keyd-application-mapper
 - Various bugfixes and stability improvements

# v2.2.5-beta

 - Eliminate udev as a dependency
 - Permit mapping to modifier key codes (still discouraged)
 - Support for nested swapping
 - Improve app detection
 - Various bug fixes

# v2.2.4-beta

  - Add support for application mapping by title
  - Fix macro timeouts
  - Forbid modifier keycodes as lone right hand values in favour of layers

# v2.2.3-beta

 - Enable hot swapping of depressed keybindings via -e
 - Improve support for application remapping

# v2.2.2-beta

 - Change panic sequence to `backspace+enter+escape` (#94)
 - Fix overload+modifer behaviour (#95)

# v2.2.1-beta

 - Move application bindings into ~/.config/keyd/app.conf.
 - Add -d to keyd-application-mapper.
 - Fix broken gnome support.

# v2.2.0-beta

 - Add a new IPC mechanism for dynamically altering the keymap (-e).
 - Add experimental support for app specific bindings.

# v2.1.0-beta

 NOTE: This might break some configs (see below)

 - Change default modifier mappings to their full names (i.e `control =
   layer(control)` instead of `control = layer(C)` in an unmodified key map)`.

 - Modifier names are now syntactic sugar for assigning both associated key
   codes.  E.G `control` corresponds to both `leftcontrol` and `rightcontrol`
   when assigned instead of just the former.  (Note: this also means it is no
   longer a valid right hand value)

 - Fixes v1 overload regression (#74)

# v2.0.1-beta

 - Add + syntax to macros to allow for simultaenously held keys.

# v2.0.0-beta

Major version update.
