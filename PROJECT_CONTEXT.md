# CabDial — a round touchscreen throttle for DCC-EX

## Latest implementation status

- Roster iteration uses `Loco::getFirst()` / `getNext()`; the user confirmed both
  roster entries and their functions print correctly.
- Reconnect handling requires a fresh version response and refreshes the roster.
  Protocol logging and reset diagnostics remain enabled.
- Function states are remembered per address for the current session. They are
  not persisted across power cycles or synchronized with other throttles.
- The LOCO/address area now opens a locomotive selector. Touch/swipe or the
  encoder browses the list; Select/short press chooses; Back/long press returns.
- Enter Address always remains available and uses the existing manual editor.
  The original short-press manual-address workflow on the throttle is retained.
- The selector uses a copied application-model roster, never library pointers.
  Selecting a roster entry copies its name and function definitions and restores
  that address's saved function states. Manual entry resets roster metadata.
- The throttle shows the selected name and first five defined roster functions;
  manual entries retain F0–F4. Roster functions beyond five are available through
  page arrows above the function row. Changing locomotives opens page 1.
- Momentary roster touchscreen functions send ON on touch press and OFF on release;
  their highlighted state exists only while held. The physical double-click for F0
  sends a short ON/OFF pulse when roster F0 is momentary because it has no paired
  release event. Latching functions retain their existing toggle behavior.
- Pending throttle changes are sent before deleting the old local Loco object,
  so a rapid selection cannot discard its queued stop.
- DCC-EX connection details are configured on the touchscreen instead of being
  compiled into the firmware. Wi-Fi SSID, Wi-Fi password, command-station IP
  address, and port are saved in ESP32 Preferences and restored at startup.
- The bottom connection-status button opens the connection editor. On a first
  run with no saved profile, the editor is mandatory until valid details are
  saved. Its status changes from Wi-Fi connection to DCC-EX connection and then
  DCC-EX connected after the version response is received.
- Incoming DCC-EX locomotive broadcasts synchronize the current locomotive's
  speed, direction, and functions with changes made by other throttles.
  Function maps for non-selected addresses are retained for later selection.
- The throttle has a dedicated turnout page. It requests named turnout entries,
  displays their closed/thrown state, sends explicit Close and Throw commands,
  and refreshes from DCC-EX turnout broadcasts.
- The Safety page provides a selected-locomotive stop, global DCC-EX emergency
  stop, and global track power controls. Turning track power off requires a
  second confirmation tap; the displayed power state follows DCC-EX broadcasts.
- Host regression tests and on-device checks are documented in `tests/README.md`.
  The new selector still needs on-device validation.

The roster-debugging notes below describe the earlier investigation.

## Goal

Build a standalone touchscreen DCC-EX throttle using an ESP32-S3
round touchscreen display.

The application will ultimately support:

- locomotive throttle control
- locomotive selection from the DCC-EX roster
- manual locomotive address selection for locomotives not in the roster
- dynamic function buttons based on the selected roster locomotive
- momentary and latching functions
- turnout list and turnout control
- physical rotary encoder/button control
- touchscreen control

## Hardware

Display:
Viewe UEDX48480021-MD80

Touch:
MD80ET

MCU:
ESP32-S3

PlatformIO board environment:
esp32s3_120_16_8-qio_opi

Encoder:
A = GPIO6
B = GPIO5
Button = GPIO0

## Software

Framework:
Arduino / PlatformIO

Display:
ESP32_Display_Panel 1.0.5

GUI:
LVGL 8.4

DCC:
DCCEXProtocol
https://github.com/DCC-EX/DCCEXProtocol

Use the main/current DCCEXProtocol library version configured in
platformio.ini.

## Architecture

Keep these layers separate:

main.cpp
  |
  +-- DccController
  |      |
  |      +-- DCCEXProtocol
  |
  +-- ThrottleUI
         |
         +-- LVGL

The application Locomotive model must remain independent of
DCCEXProtocol::Loco.

Do not couple ThrottleUI directly to DCCEXProtocol.

## Current working functionality

The following currently works and should not be broken:

- ESP32-S3/display initialization
- LVGL 8 UI
- touch
- rotary encoder
- encoder direction
- encoder push button
- WiFi connection
- TCP connection to DCC-EX Command Station
- DCCEXProtocol connection
- Command Station version retrieval
- locomotive acquisition
- speed control
- direction control
- F0-F4 function control
- physical button control
- touchscreen function control
- manual locomotive address selection
- circular throttle UI
- speed arc

Test locomotive address currently used: 101.

## Touch fix

The touch controller returned 4095/4095 continuously when not touched,
causing LVGL warnings.

This was fixed in the LVGL touch read implementation by rejecting
invalid/no-touch coordinates rather than reporting them as pressed.

Do not undo this fix.

## Application locomotive model

The application has its own Locomotive structure.

It contains approximately:

- address
- name
- fromRoster
- speed
- directionForward
- functionStates[]
- functionDefinitions[]

Function definitions contain:

- available
- momentary
- name

This model intentionally remains independent of DCCEXProtocol::Loco.

## Function UI

ThrottleUI has generic FunctionSlot objects rather than hard-coded
F0/F1/etc buttons.

There are currently 5 visible function slots.

For a manually entered locomotive:
F0-F4 are currently made available.

For a roster locomotive:
the intention is to display only functions actually defined for that
locomotive.

More than five available functions will eventually require paging or
another UI mechanism.

Function touch events are sent from:

ThrottleUI
 -> main.cpp
 -> DccController
 -> DCCEXProtocol

ThrottleUI must not directly operate DCCEXProtocol.

## DCC-EX roster

Roster integration is currently being implemented.

DCCEXProtocol documentation is in:

docs/DCCEXProtocol-Usage.pdf
docs/DCCEXProtocol-Library.pdf

Use these documents and the installed DCCEXProtocol source rather than
guessing API behavior.

Important discovery:

refreshRoster() DOES NOT directly request the roster.

Its implementation resets internal state:

    clearRoster();
    _receivedLists = false;
    _receivedRoster = false;
    _rosterRequested = false;

The documented way to retrieve only the roster is:

    dccexProtocol.getLists(true, false, false, false);

The documentation recommends calling getLists() from loop().

DccController currently wraps this as requestRoster().

Roster retrieval now works.

Latest serial output:

    Connecting to EX-CommandStation at 192.168.178.119:2560
    Connected to EX-CommandStation
    Requested EX-CommandStation version
    EX-CommandStation version: 5.9.4
    Roster received
    DCC-EX roster contains 2 locomotives
    Connecting to EX-CommandStation at 192.168.178.119:2560
    Connected to EX-CommandStation
    Requested EX-CommandStation version

So:

- roster retrieval works
- roster count is 2
- there appears to be an unexpected reconnect after roster retrieval
- roster entries themselves have not yet been successfully printed/verified

This is the current debugging point.

## DCCEXProtocol Loco API discovered

Relevant Loco methods include:

    int getAddress();
    const char *getName();

    int getSpeed();
    Direction getDirection();

    LocoSource getSource();

    bool isFunctionOn(int function);
    int getFunctionStates();

    const char *getFunctionName(int function);
    bool isFunctionMomentary(int function);

    Loco *getNext();

The library also exposes roster/list functionality including:

    getRosterCount()
    receivedRoster()
    findLocoInRoster()
    clearRoster()
    clearLocalLocos()
    refreshRoster()

Verify exact signatures against the installed library before coding.

## Immediate next task

DO NOT redesign the project.

First inspect the current source without changing it.

Determine:

1. Why the DCC-EX connection appears to reconnect immediately after
   successful roster retrieval.

2. Why getRosterCount() reports 2 locomotives but the individual
   roster entries have not yet been printed.

3. Verify correct roster iteration against the installed
   DCCEXProtocol version and documentation.

Preserve all currently working throttle functionality.

Only after the roster/reconnect issue is understood should we implement
mapping from DCCEXProtocol::Loco to the application's Locomotive model.

## Future UI

After roster retrieval is stable:

1. Build locomotive-selection screen.
2. Show DCC-EX roster locomotives.
3. Always provide "Enter Address" for a locomotive not in the roster.
4. Selecting a roster locomotive populates its function definitions.
5. Throttle screen shows only functions available for that locomotive.
6. Support momentary functions correctly.
7. Add function paging if more functions exist than visible slots.
8. Add a separate turnout screen using the turnout list reported by
   the Command Station.

The display is round, so UI layouts should be designed specifically
for the circular screen rather than as rectangular mobile layouts.
