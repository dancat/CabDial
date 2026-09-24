# Adding hardware support

The application has one hardware boundary: `Device` in `src/Device.h`.
The DCC-EX controller, connection storage, locomotive state, and UI callbacks
must not depend on a board SDK, GPIO number, display driver, or input library.

## Device implementation

Implement `Device` for a new board and select it in `DeviceFactory.cpp` using a
new `DEVICE_*` build flag. Its `begin()` method must initialise the display,
touch input, LVGL port, and any available physical inputs. It receives the
application callbacks for encoder decrease/increase, single click, double
click, and long press. An unavailable input is represented by the
corresponding capability.

`setBacklight()` must be harmless when the board has no adjustable backlight.

## Display profile

Return physical pixel dimensions and safe top/bottom margins through
`DisplayProfile`. `round` identifies screens that need corner-safe layouts.
The ViewE profile is 480 x 480, round, with top and bottom safe margins.

UI pages receive this profile before their `begin()` methods. `UiLayout` maps
the existing 480 x 480 design coordinates into a future device's usable area.
Future rectangular layouts should use this helper or a dedicated layout class,
instead of embedding new physical display dimensions in UI code.

## PlatformIO environment

Put dependencies used by every board under `[common]` in `platformio.ini`.
Put the board definition, display-driver libraries, input libraries, and board
macros in its own `[env:...]` section. The current ViewE environment remains
the only build target until another board is intentionally added.
