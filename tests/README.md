# Host regression tests

These standalone tests do not require a connected board. The model test uses a
minimal Arduino type shim; it does not emulate hardware or the DCC protocol.

From the project root:

```sh
c++ -std=c++11 -Wall -Wextra -pedantic -Isrc tests/locomotive_function_states_test.cpp -o /tmp/locomotive_function_states_test
/tmp/locomotive_function_states_test
c++ -std=c++17 -Wall -Wextra -pedantic -Itests/support -Isrc tests/locomotive_definition_test.cpp -o /tmp/locomotive_definition_test
/tmp/locomotive_definition_test
c++ -std=c++11 -Wall -Wextra -pedantic tests/function_paging_test.cpp -o /tmp/function_paging_test
/tmp/function_paging_test
```

On-device selection checks:

- Tap LOCO/address to open the selector. Swipe or turn the encoder; use Select or
  short press to choose. Back or long press returns to the throttle, still stopped.
- Select a roster entry: check its name/address and first five named functions.
- Hold a momentary touchscreen function: it should be active only while held.
  A physical double-click on momentary F0 sends a short pulse because it has no
  matching release action.
- For more than five defined roster functions, use the arrows above the function
  row to change pages. A newly selected locomotive opens on page 1.
- Choose Enter Address: check manual encoder selection and F0–F4 are restored.
- Set different functions on two locomotives and switch between them; check that
  each retains its own function states.
- Open the selector with an empty roster or while disconnected: Enter Address
  remains available. On reconnect, the selector repopulates from a fresh snapshot.
- Switch quickly after opening the selector: verify the old address receives its
  pending stop command before selecting the new address.

On-device connection setup checks:

- On a first run, the connection editor opens and cannot be dismissed until a
  Wi-Fi network, DCC-EX IP address, and port are provided.
- Enter the Wi-Fi password if the network needs one, then use **Save & Connect**.
  Confirm the bottom status button progresses to **DCC-EX CONNECTED**.
- Tap the status button to reopen the editor, save an updated value, then restart
  the throttle. It should reconnect with the saved profile.
- In the connection editor, choose **Refresh Lists** after a Command Station
  configuration change. Confirm the roster and turnout pages reload their data.
- With two throttles controlling the same address, change speed, direction, or a
  function on one. The other throttle should update from the Command Station's
  locomotive broadcast. Function states for a different address are retained
  until that address is selected here.
- Tap **TURNOUTS** on the throttle screen. Confirm the named turnout list and
  its states load, then choose a turnout and use **Close** or **Throw**. Verify
  the Command Station responds and a change from another throttle refreshes the
  displayed state. The encoder moves selection; a short press throws and a long
  press returns to the throttle screen.
- Tap **SAFE**. Confirm **STOP LOCO** stops only the displayed locomotive and
  **EMERGENCY STOP** sends the Command Station's global emergency stop. Verify
  the track-power state follows the Command Station. With power on, the first
  **POWER OFF** tap must change to **CONFIRM POWER OFF**; only the second tap
  turns off global track power.

Momentary touch behavior and function paging are subsequent steps; this selection
change copies those definitions but retains the existing toggle interaction.
