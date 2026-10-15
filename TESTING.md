# sokketter Testing Plan

This document defines how to verify **sokketter** before a release. It has two parts:

- **[Part 1 — Manual Test Plan](#part-1--manual-test-plan)** — a pre-release checklist covering every
  user-facing feature of the three first-party components (`libsokketter`, `sokketter-cli`,
  `sokketter-ui`) on Windows, Linux, and macOS.
- **[Part 2 — Automated Test Plan](#part-2--automated-test-plan)** — a testing strategy derived from
  the manual plan, with concrete tooling recommendations and a prioritized roadmap.

The scope is the current functionality only. Supported devices, platforms, and the public API are
described in [README.md](README.md) and [AGENTS.md](AGENTS.md).

---

## Components under test

| Component | Description | Key sources |
| --- | --- | --- |
| `libsokketter` | Core C++17 library. Public API only. | [libsokketter/include/libsokketter.h](libsokketter/include/libsokketter.h) |
| `sokketter-cli` | CLI11 command-line front end. | [sokketter-cli/sources/cli_parser.cpp](sokketter-cli/sources/cli_parser.cpp) |
| `sokketter-ui` | Qt6 Widgets GUI. | [sokketter-ui/sources/mainwindow.cpp](sokketter-ui/sources/mainwindow.cpp) |

### Supported devices

| Device                | Bus      | VID:PID / identity                | Sockets | Auth     |
| --------------------- | -------- | --------------------------------- | ------- | -------- |
| Gembird MSIS-PM       | USB      | `0x04b4:0xfd10`                   | 1       | none     |
| Gembird MSIS-PM2      | USB      | `0x04b4:0xfd12`                   | 1       | none     |
| Gembird SIS-PM        | USB      | `0x04b4:0xfd11`                   | 4       | none     |
| Energenie EG-PMS      | USB      | `0x04b4:0xfd13`                   | 4       | none     |
| Energenie EG-PMS2     | USB      | `0x04b4:0xfd15`                   | 4       | none     |
| Energenie EG-PMxx-LAN | Ethernet | TCP port `5000`, MAC `88:B6:27:*` | 4       | password |

---

## Test environment setup

### Storage / state locations

Persisted state must be inspected and reset between cases. Delete these files to start clean.

| Platform | Storage path | Files |
| --- | --- | --- |
| Windows | `C:\ProgramData\sokketter` | `devices.json`, `sokketter-ui.json`, `logs\` |
| macOS | `/Users/Shared/sokketter` | `devices.json`, `sokketter-ui.json`, `logs/` |
| Linux | `~/.local/share/sokketter` | `devices.json`, `sokketter-ui.json`, `logs/` |

### Linux USB permissions

USB device access on Linux requires the udev rules in [udev-rules/](udev-rules); install them with
[udev-rules/install-udev-rules.sh](udev-rules/install-udev-rules.sh) and replug the device before
running USB cases.

### Result legend

Each execution table has one result column per supported OS: **Result: Windows** (Windows 10/11),
**Result: Linux** (Ubuntu 20.04+), **Result: macOS** (macOS 10.15+ Apple Silicon). 

Fill each cell with one of: ✅ Pass · ❌ Fail · ⚠️ Blocked/NA · ⬜ Not run. 

Note the build flavor (static/shared) and app version (`--version` / About page) with the run. 
Mark a cell ⚠️ when a case does not apply to that platform (e.g. Windows-only title-bar theming).

---

# Part 1 — Manual Test Plan

Test-case IDs are prefixed `MAN-`. Unless a case is device-specific (group E) or hardware-only, run it
with fake devices on each supported OS and record the outcome in the matching result column.

## A. Command-line interface (`sokketter-cli`)

Verify both `stdout`/`stderr` content and the process exit code. 

> **Exit-code contract:** the CLI collapses every CLI11 parse-error code (100+) onto the documented
> contract — **`0`** for success (including `--help`/`--version`) and **`1`** for any failure. See
> [cli_parser.cpp](sokketter-cli/sources/cli_parser.cpp) `application.exit()` handling. Do **not**
> expect CLI11's raw `106`/`108`/`109` codes.

| ID         | Command                                               | Expected                                                                                                                                                                     | Result: Windows | Result: Linux | Result: macOS |
| ---------- | ----------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------- | ------------- | ------------- |
| MAN-CLI-01 | `sokketter-cli` (no args)                             | Exit `1`; stderr `A subcommand is required`.                                                                                                                                 | ⬜               | ⬜             | ⬜             |
| MAN-CLI-02 | `sokketter-cli --help` / `-h`                         | Exit `0`; prints the descriptive help text.                                                                                                                                  | ⬜               | ⬜             | ⬜             |
| MAN-CLI-03 | `sokketter-cli --version` / `-v`                      | Exit `0`; prints `sokketter-cli version …`.                                                                                                                                  | ⬜               | ⬜             | ⬜             |
| MAN-CLI-04 | `sokketter-cli list` (no devices)                     | Exit `0`; **stdout** `Listing available devices…` then `No devices found.` (empty bus is a valid answer, not a failure — scripts must match the message, not the exit code). | ⬜               | ⬜             | ⬜             |
| MAN-CLI-05 | `sokketter-cli list` (N fake devices)                 | Exit `0`; numbered device list printed.                                                                                                                                      | ⬜               | ⬜             | ⬜             |
| MAN-CLI-06 | `sokketter-cli list power` / `power list`             | Exit `1`; stderr `The following argument was not expected: …`.                                                                                                               | ⬜               | ⬜             | ⬜             |
| MAN-CLI-07 | `power status -i 1 -s 1`                              | Exit `0`; prints device + `Socket 1: … status: off`.                                                                                                                         | ⬜               | ⬜             | ⬜             |
| MAN-CLI-08 | `power status -n <serial> -s 1`                       | Exit `0`; same output selected by serial.                                                                                                                                    | ⬜               | ⬜             | ⬜             |
| MAN-CLI-09 | `power status -i 1 -n <serial>`                       | Exit `1`; stderr `--device-at-index excludes --device-with-serial`.                                                                                                          | ⬜               | ⬜             | ⬜             |
| MAN-CLI-10 | `power status` (no device flag)                       | Exit `1`; stderr `[Option Group: …] is required.`.                                                                                                                           | ⬜               | ⬜             | ⬜             |
| MAN-CLI-11 | `power status -n <missing>`                           | Exit `1`; stderr `No device was found for serial "<missing>".`.                                                                                                              | ⬜               | ⬜             | ⬜             |
| MAN-CLI-12 | `power on -i 1` (no `--sockets`)                      | Exit `0`; **all** sockets report `turned on.`.                                                                                                                               | ⬜               | ⬜             | ⬜             |
| MAN-CLI-13 | `power on -i 1 -s 1`                                  | Exit `0`; only socket 1 reports `turned on.`.                                                                                                                                | ⬜               | ⬜             | ⬜             |
| MAN-CLI-14 | `power off -i 1 -s 1 2`                               | Exit `0`; sockets 1 and 2 report `turned off.`.                                                                                                                              | ⬜               | ⬜             | ⬜             |
| MAN-CLI-15 | `power toggle -i 1 -s 1` twice                        | Exit `0`; socket returns to its original state after two toggles.                                                                                                            | ⬜               | ⬜             | ⬜             |
| MAN-CLI-16 | `power status -i 1 -s 0` and `-s 99`                  | Exit `1`; stderr `Socket index … is out of range (valid range: 1-N).` (1-based indices).                                                                                     | ⬜               | ⬜             | ⬜             |
| MAN-CLI-17 | `power on --help` (and status/off/toggle)             | Exit `0`; help text shown (help precedence over subcommand).                                                                                                                 | ⬜               | ⬜             | ⬜             |
| MAN-CLI-18 | Windows-style `/help`, mixed case `LIST`, underscores | Parsed the same as canonical forms (`ignore_case` / `ignore_underscore` / windows options).                                                                                  | ⬜               | ⬜             | ⬜             |
| MAN-CLI-19 | `sokketter-cli list --include-device-types ethernet`  | Exit `0`; Ethernet/LAN include filter is accepted. With fake devices the list is unchanged (Test devices stay listed regardless of selection).                               | ⬜               | ⬜             | ⬜             |
| MAN-CLI-20 | `sokketter-cli list --include-device-types lan`       | Exit `0`; LAN alias is accepted and behaves the same as `ethernet`.                                                                                                          | ⬜               | ⬜             | ⬜             |
| MAN-CLI-21 | `power status -i 0`                                   | Exit `1`; stderr `Device index 0 is out of range (valid range starts from 1).`.                                                                                              | ⬜               | ⬜             | ⬜             |
| MAN-CLI-22 | `power status -i 1 -t usb`                            | Exit `0`; `--include-device-types` is accepted on the `power` subcommand and resolves index 1 against the USB selection.                                                     | ⬜               | ⬜             | ⬜             |
| MAN-CLI-23 | `power status -i 1 -t ethernet` (fake devices)        | Exit `0`; Test devices stay addressable by index even under an Ethernet-only selection.                                                                                      | ⬜               | ⬜             | ⬜             |
| MAN-CLI-24 | `power status -n TEST_SERIAL_NUMBER_0 -t ethernet`    | Exit `0`; Test devices stay addressable by serial even under an Ethernet-only selection.                                                                                     | ⬜               | ⬜             | ⬜             |
| MAN-CLI-25 | `power status -i 1 -t garbage`                        | Exit `1`; stderr `Unknown device type: garbage. Available types are: USB, ETHERNET, LAN.`; **no** update-notification line (validation runs before the update check).       | ⬜               | ⬜             | ⬜             |
| MAN-CLI-26 | `list --include-device-types ,usb`                    | Exit `1`; stderr `Empty device type in --include-device-types. …` (leading comma → empty token; likewise a trailing comma or an empty value).                               | ⬜               | ⬜             | ⬜             |
| MAN-CLI-27 | `list --include-device-types " "`                     | Exit `1`; stderr `Empty device type in --include-device-types. …` (whitespace-only token).                                                                                   | ⬜               | ⬜             | ⬜             |
| MAN-CLI-28 | `list --include-device-types=usb`                     | Exit `0`; `=`-form accepted (normalization stops at `=`).                                                                                                                    | ⬜               | ⬜             | ⬜             |
| MAN-CLI-29 | `power status -i 1` (no devices)                      | Exit `1`; stderr `Device index 1 is out of range (no devices are available).`.                                                                                               | ⬜               | ⬜             | ⬜             |
| MAN-CLI-30 | `power status -n <real USB serial> -t ethernet`       | Exit `1`; stderr `No device was found for serial "<serial>".` (serial outside the selection is reported as not found).                                                       | ⬜               | ⬜             | ⬜             |
| MAN-CLI-31 | `list` with `LIBSOKKETTER_TEST_DEVICE_NUMBER=N`       | Fake-device naming contract: entries are `Test Device <i>` with serial `TEST_SERIAL_NUMBER_<i>` and address `TEST_ADDRESS_<i>`, where `i` is **0-based**.                    | ⬜               | ⬜             | ⬜             |
| MAN-CLI-32 | `power status -i 1 -s 4` then `-s 5` (4-socket device) | `-s 4` exits `0`; `-s 5` exits `1` with an out-of-range message naming the valid range. On a 1-socket device the range is printed degenerate as `1-1`.                       | ⬜               | ⬜             | ⬜             |
| MAN-CLI-33 | `power status -i N` then `-i N+1` (N devices present) | `-i N` exits `0`; `-i N+1` exits `1` with an out-of-range message naming the valid range.                                                                                    | ⬜               | ⬜             | ⬜             |
| MAN-CLI-34 | `power status -i 18446744073709551615` / `…616`       | `SIZE_MAX` parses and is reported out of range; `SIZE_MAX + 1` exits `1` with an argument-conversion error.                                                                   | ⬜               | ⬜             | ⬜             |
| MAN-CLI-35 | `power off -i 1 -s 2 1` and `-s 1 1 2 2`              | Exit `0`; sockets are reported in **sorted** order, and repeated indices are applied **once** (one `turned off.` line per socket).                                           | ⬜               | ⬜             | ⬜             |
| MAN-CLI-36 | `list list` / `power status status`                   | Exit `1`; stderr `The following argument was not expected: …`.                                                                                                              | ⬜               | ⬜             | ⬜             |
| MAN-CLI-37 | `list` with invalid/huge `LIBSOKKETTER_TEST_DEVICE_NUMBER` | The value is treated as unset — no fake devices, same output as MAN-CLI-04.                                                                                              | ⬜               | ⬜             | ⬜             |

## B. Graphical interface (`sokketter-ui`)

Run with fake devices unless a case is hardware-specific.

### B1. Device list & navigation

| ID        | Steps                                                              | Expected                                                            | Result: Windows | Result: Linux | Result: macOS |
| --------- | ------------------------------------------------------------------ | ------------------------------------------------------------------- | --------------- | ------------- | ------------- |
| MAN-UI-01 | Launch the app.                                                    | Main window opens at the persisted geometry.                        | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-02 | With no devices present.                                           | The empty-list placeholder item is shown; clicking it does nothing. | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-03 | Click the refresh control.                                         | Device list re-enumerates.                                          | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-04 | Resize the window.                                                 | List items reflow to the available width without clipping.          | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-05 | Toggle **USB devices** / **Ethernet devices** filters in Settings. | List updates to include only the allowed bus types.                 | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-06 | Observe a disconnected saved device.                               | Item shows disconnected state and is not interactive for switching. | ⬜                      | ⬜                    | ⬜                   |

### B2. Authentication

| ID        | Steps                                                                                                          | Expected                                                                                              | Result: Windows | Result: Linux | Result: macOS |
| --------- | -------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------- | --------------- | ------------- | ------------- |
| MAN-UI-07 | Click a device with **no** auth (USB/fake).                                                                    | Goes straight to the socket list.                                                                     | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-08 | Click an EG-PMxx-LAN device (password auth).                                                                   | Redirected to the password page.                                                                      | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-09 | Use the back control on the auth page.                                                                         | Returns to the device list.                                                                           | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-10 | Enter a wrong password.                                                                                        | `Authentication failed! Please try again.`; stays on the page.                                        | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-11 | Enter the correct password and log in (button or Enter).                                                       | `Authentication succeed!`; socket list opens; password saved.                                         | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-12 | After authenticating for the first time, visit the device page, go back and open the same device               | Entering password once again is not required                                                          | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-13 | Click a **connected** device with no auth or valid saved credentials.                                          | `try_authenticate()` runs on the worker thread; on success the socket list opens.                     | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-14 | Click a saved but **offline/disconnected** device (no auth or valid saved credentials).                        | Skips authentication and proceeds straight to the socket list; no connection attempt is forced.       | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-15 | Click a saved but **offline/disconnected** device (with auth but without valid saved credentials).             | Sockets are disabled (cannot toggle while offline), but configuration is still editable and persists. | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-16 | On an **offline** device's socket list, open the edit → Configure page, change device/socket fields, and Save. | Sockets are disabled (cannot toggle while offline), but configuration is still editable and persists. | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-17 | Reconnect a device whose saved password no longer works.                                                       | Auto-redirect back to the authentication page.                                                        | ⬜                      | ⬜                    | ⬜                   |

### B3. Socket control

| ID        | Steps                                                                             | Expected                                                                                  | Result: Windows | Result: Linux | Result: macOS |
| --------- | --------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------- | --------------- | ------------- | ------------- |
| MAN-UI-18 | In **single-click** mode, click a socket.                                         | Socket toggles; status LED updates after the async operation.                             | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-19 | For a socket with a configurable reset, press reset.                              | Socket powers off, waits the configured ms, powers back on; button disabled during reset. | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-20 | Proceed to LAN device settings, change anything, save and go back to socket list. | Socket are having the same status as before (ON or OFF) and not UNKNOWN                   | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-21 | Socket with reset = 0 ms.                                                         | Reset button is hidden.                                                                   | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-22 | Rapidly toggle multiple sockets.                                                  | UI stays responsive (I/O on worker thread); no stuck/disabled sockets.                    | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-23 | Switch to **double-click** mode in Settings, single-click a socket.               | Single click does nothing; double click toggles.                                          | ⬜                      | ⬜                    | ⬜                   |

### B4. Configuration

| ID        | Steps                                                              | Expected                                                                | Result: Windows | Result: Linux | Result: macOS |
| --------- | ------------------------------------------------------------------ | ----------------------------------------------------------------------- | --------------- | ------------- | ------------- |
| MAN-UI-24 | Open the socket list edit → Configure page.                        | Device edit form + one form per socket are shown.                       | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-25 | Use Configure/back controls.                                       | Navigation returns without saving.                                      | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-26 | Change device name/description and Save.                           | New values persist and appear in the device/socket lists after restart. | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-27 | Change socket name/description and Save.                           | New values persist.                                                     | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-28 | Set a socket configurable-reset value (validated integer).         | Only non-negative integers accepted; reset button appears when > 0.     | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-29 | Delete/forget the device, choose **No**.                           | Nothing is deleted.                                                     | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-30 | Delete/forget the device, confirm the warning dialog with **Yes**. | Device removed from list and `devices.json`; returns to device list.    | ⬜                      | ⬜                    | ⬜                   |

### B5. Settings, About & theme

| ID        | Steps                                                        | Expected                                                                     | Result: Windows | Result: Linux | Result: macOS |
| --------- | ------------------------------------------------------------ | ---------------------------------------------------------------------------- | --------------- | ------------- | ------------- |
| MAN-UI-31 | Open Settings; click "open data folder".                     | Storage folder opens in the OS file manager.                                 | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-32 | Change theme radio (Auto/Light/Dark).                        | Whole app (incl. dialogs, list items) restyles live.                         | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-33 | With theme = Auto, change the OS light/dark mode.            | App follows the OS theme; Windows title bar switches; macOS detection fires. | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-34 | Open About.                                                  | Shows version, git hash, build date, license line, and components list.      | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-35 | Open the License dialog; switch tabs.                        | Dialog opens, tabs render, OK closes it; respects the current theme.         | ⬜                      | ⬜                    | ⬜                   |
| MAN-UI-36 | Change UI settings, move/resize the window, close, relaunch. | Geometry restored from `sokketter-ui.json`.                                  | ⬜                      | ⬜                    | ⬜                   |

## C. Device-specific

Run per attached device. Verify the device is matched to the correct class and that physical switching
works. Confirm the **socket count** matches the table above.

| ID         | Device              | Steps                                                                 | Expected                                                                           | Result: Windows | Result: Linux | Result: macOS |
| ---------- | ------------------- | --------------------------------------------------------------------- | ---------------------------------------------------------------------------------- | --------------- | ------------- | ------------- |
| MAN-DEV-01 | Gembird SIS-PM      | Plug in; `list`; toggle each of 4 sockets.                            | Detected as `Gembird SIS-PM`; 4 sockets switch physically; status reads back.      | ⬜                      | ⬜                    | ⬜                   |
| MAN-DEV-02 | Gembird MSIS-PM     | Plug in; toggle the single socket.                                    | Detected as `Gembird MSIS-PM`; 1 socket switches.                                  | ⬜                      | ⬜                    | ⬜                   |
| MAN-DEV-03 | Gembird MSIS-PM (2) | Plug in; toggle the single socket.                                    | Detected as `Gembird MSIS-PM (2)`; 1 socket switches.                              | ⬜                      | ⬜                    | ⬜                   |
| MAN-DEV-04 | Energenie EG-PMS    | Plug in; toggle each of 4 sockets.                                    | Detected as `Energenie EG-PMS`; serial read from device; 4 sockets switch.         | ⬜                      | ⬜                    | ⬜                   |
| MAN-DEV-05 | Energenie EG-PMS2   | Plug in; toggle each of 4 sockets.                                    | Detected as `Energenie EG-PMS2`; 4 sockets switch.                                 | ⬜                      | ⬜                    | ⬜                   |
| MAN-DEV-06 | EG-PMxx-LAN         | Discover on LAN; authenticate; toggle each of 4 sockets; read status. | Detected via MAC/port; password login works; 4 sockets switch; status echoed back. | ⬜                      | ⬜                    | ⬜                   |
| MAN-DEV-07 | Any USB             | Unplug the device mid-session.                                        | Device shown as disconnected; operations fail gracefully; no crash.                | ⬜                      | ⬜                    | ⬜                   |
| MAN-DEV-08 | Any LAN             | Unplug the device mid-session.                                        | Device shown as disconnected; operations fail gracefully; no crash.                | ⬜                      | ⬜                    | ⬜                   |
| MAN-DEV-09 | Linux, any USB      | Run without udev rules, then with them.                               | Without rules: access denied/enumeration fails; with rules: works.                 | ⚠️                     | ⬜                    | ⚠️                  |

## D. Persistence & migration

| ID         | Steps                                                     | Expected                                                                              | Result: Windows | Result: Linux | Result: macOS |
| ---------- | --------------------------------------------------------- | ------------------------------------------------------------------------------------- | --------------- | ------------- | ------------- |
| MAN-PER-01 | Connect a new device, then inspect `devices.json`.        | Device appended with type/id/name/description/auth/sockets.                           | ⬜                      | ⬜                    | ⬜                   |
| MAN-PER-02 | Edit any config in application, restart the app.          | Edited values survive the restart.                                                    | ⬜                      | ⬜                    | ⬜                   |
| MAN-PER-03 | Enumerate with fake devices, inspect `devices.json`.      | Test devices are **not** persisted.                                                   | ⬜                      | ⬜                    | ⬜                   |
| MAN-PER-04 | Load an old `devices.json` with no `authentication-type`. | Loads without error; auth type defaults from the device class (backwards compatible). | ⬜                      | ⬜                    | ⬜                   |
| MAN-PER-05 | Corrupt/partial `devices.json`.                           | App logs an error and starts with a usable (empty) database instead of crashing.      | ⬜                      | ⬜                    | ⬜                   |

---

# Part 2 — Automated Test Plan

The automated suite mirrors the manual plan as a testing pyramid: a broad base of fast library unit
tests, a middle layer of CLI integration tests, and a thin top layer of GUI automation. Test-case IDs
are prefixed `AUTO-`.

## Current state

| Suite            | Location                                               | Status                                                                                                                                                                                                                                                     |
| ---------------- | ------------------------------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| CLI parser tests | [sokketter-cli/tests](sokketter-cli/tests)             | Present — options, subcommands, general parsing, plus full `MAN-CLI-01`…`MAN-CLI-37` coverage (short flags, mixed case, windows-style, underscore aliasing, dedup, validate-before-hardware, actions via serial, multi-socket toggle, conversion errors). |
| UI tests         | —                                                      | **Missing**.                                                                                                                                                                                                                                               |
| kommpot tests    | [third-party/kommpot/tests](third-party/kommpot/tests) | Vendored; do not edit.                                                                                                                                                                                                                                     |

## Recommended tooling

| Layer                     | Tool                                                       | Rationale                                                                                       |
| ------------------------- | ---------------------------------------------------------- | ----------------------------------------------------------------------------------------------- |
| CLI unit + integration    | **GoogleTest + GMock**                                     | Already fetched (v1.16.0) and wired via `gtest_discover_tests`; matches existing CLI tests.     |
| GUI (widget-level)        | **Qt Test (`QtTest`)**                                     | Free, ships with Qt6; `QTest::mouseClick`, `QSignalSpy`, and event injection cover the widgets. |
| GUI (optional end-to-end) | **Squish for Qt**                                          | Commercial; only if scripted full-app E2E is required beyond Qt Test.                           |
| Coverage                  | **lcov/gcov** (Linux/macOS), **OpenCppCoverage** (Windows) | `get_coverage.sh` already uses lcov; OpenCppCoverage fills the Windows gap.                     |
| Memory / UB               | **ASan + UBSan** (Clang/GCC)                               | Catch leaks and UB in device/serialization code on Linux/macOS.                                 |
| Static analysis           | **clang-tidy + cppcheck**                                  | Enforce the project C++ conventions and catch defects pre-merge.                                |
| CI orchestration          | **GitHub Actions** via [Build.py](Build.py)                | Existing pipeline; extend with a test matrix across the three platforms.                        |

## L1 — UI automation (new Qt Test target)

Widget-level tests with `QtTest`, driving fake devices via the env var. Keep them headless-capable
(offscreen platform plugin) for CI.

| ID | Under test | Derived from | Notes |
| --- | --- | --- | --- |
| AUTO-UI-01 | Device list population + empty placeholder | MAN-UI-01/02 | Assert item widgets after enumeration. |
| AUTO-UI-02 | Bus filter behavior | MAN-UI-05 | Toggle checkboxes → `repopulate_device_list`. |
| AUTO-UI-03 | Socket toggle single vs double click | MAN-UI-13/14 | `QTest::mouseClick` / `mouseDClick`; `QSignalSpy` on state change. |
| AUTO-UI-04 | Configurable-reset flow | MAN-UI-15/16 | `toggleResetButton` signal; button hidden at 0 ms. |
| AUTO-UI-05 | `DeviceEditForm` / `SocketEditForm` round-trip | MAN-UI-19/20/21 | Set fields → `configuration()` returns them; integer validator enforced. |
| AUTO-UI-06 | Settings persistence | MAN-UI-26/31 | `app_settings_storage` round-trip for toggle/theme/geometry/filters. |
| AUTO-UI-07 | Theme switch | MAN-UI-27 | Stylesheet changes on `ThemeChange`. |
| AUTO-UI-08 | Auth page routing | MAN-UI-07/08 | No-auth → socket list; password device → auth page. |
| AUTO-UI-09 | Offline-device click routing | MAN-UI-33/35 | Offline saved device → socket list without `try_authenticate()`; connected device with failing auth → auth page. Needs a saved-but-disconnected fixture (fake test devices always report connected). |
