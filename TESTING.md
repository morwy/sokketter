# sokketter Testing Plan

This document defines how to verify **sokketter** before a release.
The scope is the current functionality only. Supported devices, platforms, and the public API are
described in [README.md](README.md) and [AGENTS.md](AGENTS.md).

---

## Components under test

| Component       | Description                          | Key sources                                                                  |
| --------------- | ------------------------------------ | ---------------------------------------------------------------------------- |
| `sokketter-cli` | CLI11 command-line front end.        | [sokketter-cli/sources/cli_parser.cpp](sokketter-cli/sources/cli_parser.cpp) |
| `sokketter-ui`  | Qt6 Widgets GUI.                     | [sokketter-ui/sources/mainwindow.cpp](sokketter-ui/sources/mainwindow.cpp)   |

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

# Test cases

Run tests case on each supported OS and record the outcome in the matching result column.

## A. Command-line interface (`sokketter-cli`)

Verify both `stdout`/`stderr` content and the process exit code. The cases are split into two groups:
**parsing** (`CLI-PRS-*`) — everything that runs with **no device online** (help/version, option
validation, and errors raised before any device is resolved); and **hardware** (`CLI-HW-*`) — cases
that require at least one real device to be online.

> **Exit-code contract:** the CLI collapses every CLI11 parse-error code (100+) onto the documented
> contract — **`0`** for success (including `--help`/`--version`) and **`1`** for any failure. See
> [cli_parser.cpp](sokketter-cli/sources/cli_parser.cpp) `application.exit()` handling. Do **not**
> expect CLI11's raw `106`/`108`/`109` codes.

### A1. Parsing (no devices required)

| ID         | Command                                                | Expected                                                                                                                                                                     | Result: Windows | Result: Linux | Result: macOS |
| ---------- | ------------------------------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------- | ------------- | ------------- |
| CLI-PRS-01 | `sokketter-cli` (no args)                              | Exit `1`; stderr `A subcommand is required`.                                                                                                                                 | ⬜               | ⬜             | ⬜             |
| CLI-PRS-02 | `sokketter-cli --help` / `-h`                          | Exit `0`; prints the descriptive help text.                                                                                                                                  | ⬜               | ⬜             | ⬜             |
| CLI-PRS-03 | `sokketter-cli --version` / `-v`                       | Exit `0`; prints `sokketter-cli version …`.                                                                                                                                  | ⬜               | ⬜             | ⬜             |
| CLI-PRS-04 | `sokketter-cli list` (no devices)                      | Exit `0`; **stdout** `Listing available devices…` then `No devices found.` (empty bus is a valid answer, not a failure — scripts must match the message, not the exit code). | ⬜               | ⬜             | ⬜             |
| CLI-PRS-05 | `sokketter-cli list power` / `power list`              | Exit `1`; stderr `The following argument was not expected: …`.                                                                                                               | ⬜               | ⬜             | ⬜             |
| CLI-PRS-06 | `power status -i 1 -n <serial>`                        | Exit `1`; stderr `--device-at-index excludes --device-with-serial`.                                                                                                          | ⬜               | ⬜             | ⬜             |
| CLI-PRS-07 | `power status` (no device flag)                        | Exit `1`; stderr `[Option Group: …] is required.`.                                                                                                                           | ⬜               | ⬜             | ⬜             |
| CLI-PRS-08 | `power status -n <missing>`                            | Exit `1`; stderr `No device was found for serial "<missing>".`.                                                                                                              | ⬜               | ⬜             | ⬜             |
| CLI-PRS-09 | `power on --help` (and status/off/toggle)              | Exit `0`; help text shown (help precedence over subcommand).                                                                                                                 | ⬜               | ⬜             | ⬜             |
| CLI-PRS-10 | Windows-style `/help`, mixed case `LIST`, underscores  | Parsed the same as canonical forms (`ignore_case` / `ignore_underscore` / windows options).                                                                                  | ⬜               | ⬜             | ⬜             |
| CLI-PRS-11 | `sokketter-cli list --include-device-types ethernet`   | Exit `0`; Ethernet/LAN include filter is accepted.                                                                                                                           | ⬜               | ⬜             | ⬜             |
| CLI-PRS-12 | `sokketter-cli list --include-device-types lan`        | Exit `0`; LAN alias is accepted and behaves the same as `ethernet`.                                                                                                          | ⬜               | ⬜             | ⬜             |
| CLI-PRS-13 | `power status -i 0`                                    | Exit `1`; stderr `Device index 0 is out of range (valid range starts from 1).`.                                                                                              | ⬜               | ⬜             | ⬜             |
| CLI-PRS-14 | `power status -i 1 -t garbage`                         | Exit `1`; stderr `Unknown device type: garbage. Available types are: USB, ETHERNET, LAN.`; **no** update-notification line (validation runs before the update check).        | ⬜               | ⬜             | ⬜             |
| CLI-PRS-15 | `list --include-device-types ,usb`                     | Exit `1`; stderr `Empty device type in --include-device-types. …` (leading comma → empty token; likewise a trailing comma or an empty value).                                | ⬜               | ⬜             | ⬜             |
| CLI-PRS-16 | `list --include-device-types " "`                      | Exit `1`; stderr `Empty device type in --include-device-types. …` (whitespace-only token).                                                                                   | ⬜               | ⬜             | ⬜             |
| CLI-PRS-17 | `list --include-device-types=usb`                      | Exit `0`; `=`-form accepted (normalization stops at `=`).                                                                                                                    | ⬜               | ⬜             | ⬜             |
| CLI-PRS-18 | `power status -i 1` (no devices)                       | Exit `1`; stderr `Device index 1 is out of range (no devices are available).`.                                                                                               | ⬜               | ⬜             | ⬜             |
| CLI-PRS-19 | `power status -i 18446744073709551615` / `…616`        | `SIZE_MAX` parses and is reported out of range; `SIZE_MAX + 1` exits `1` with an argument-conversion error.                                                                  | ⬜               | ⬜             | ⬜             |
| CLI-PRS-20 | `list list` / `power status status`                    | Exit `1`; stderr `The following argument was not expected: …`.                                                                                                               | ⬜               | ⬜             | ⬜             |

### A2. Hardware (devices are required)

| ID        | Command                                                | Expected                                                                                                                                               | Result: Windows | Result: Linux | Result: macOS |
| --------- | ------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------ | --------------- | ------------- | ------------- |
| CLI-HW-01 | `sokketter-cli list` (N devices)                       | Exit `0`; numbered device list printed.                                                                                                                | ⬜               | ⬜             | ⬜             |
| CLI-HW-02 | `power status -i 1 -s 1`                               | Exit `0`; prints device + `Socket 1: … status: off`.                                                                                                   | ⬜               | ⬜             | ⬜             |
| CLI-HW-03 | `power status -n <serial> -s 1`                        | Exit `0`; same output selected by serial.                                                                                                              | ⬜               | ⬜             | ⬜             |
| CLI-HW-04 | `power on -i 1` (no `--sockets`)                       | Exit `0`; **all** sockets report `turned on.`.                                                                                                         | ⬜               | ⬜             | ⬜             |
| CLI-HW-05 | `power on -i 1 -s 1`                                   | Exit `0`; only socket 1 reports `turned on.`.                                                                                                          | ⬜               | ⬜             | ⬜             |
| CLI-HW-06 | `power off -i 1 -s 1 2`                                | Exit `0`; sockets 1 and 2 report `turned off.`.                                                                                                        | ⬜               | ⬜             | ⬜             |
| CLI-HW-07 | `power toggle -i 1 -s 1` twice                         | Exit `0`; socket returns to its original state after two toggles.                                                                                      | ⬜               | ⬜             | ⬜             |
| CLI-HW-08 | `power status -i 1 -s 0` and `-s 99`                   | Exit `1`; stderr `Socket index … is out of range (valid range: 1-N).` (1-based indices).                                                               | ⬜               | ⬜             | ⬜             |
| CLI-HW-09 | `power status -i 1 -t usb`                             | Exit `0`; `--include-device-types` is accepted on the `power` subcommand and resolves index 1 against the USB selection.                               | ⬜               | ⬜             | ⬜             |
| CLI-HW-10 | `power status -i 1 -t ethernet` (devices)              | Exit `0`; Test devices stay addressable by index even under an Ethernet-only selection.                                                                | ⬜               | ⬜             | ⬜             |
| CLI-HW-11 | `power status -n <real serial> -t ethernet`            | Exit `0`; Test devices stay addressable by serial even under an Ethernet-only selection.                                                               | ⬜               | ⬜             | ⬜             |
| CLI-HW-12 | `power status -n <real USB serial> -t ethernet`        | Exit `1`; stderr `No device was found for serial "<serial>".` (serial outside the selection is reported as not found).                                 | ⬜               | ⬜             | ⬜             |
| CLI-HW-13 | `power status -i 1 -s 4` then `-s 5` (4-socket device) | `-s 4` exits `0`; `-s 5` exits `1` with an out-of-range message naming the valid range. On a 1-socket device the range is printed degenerate as `1-1`. | ⬜               | ⬜             | ⬜             |
| CLI-HW-14 | `power status -i N` then `-i N+1` (N devices present)  | `-i N` exits `0`; `-i N+1` exits `1` with an out-of-range message naming the valid range.                                                              | ⬜               | ⬜             | ⬜             |
| CLI-HW-15 | `power off -i 1 -s 2 1` and `-s 1 1 2 2`               | Exit `0`; sockets are reported in **sorted** order, and repeated indices are applied **once** (one `turned off.` line per socket).                     | ⬜               | ⬜             | ⬜             |
| CLI-HW-16 | Connect a new device, then inspect `devices.json`.     | Device appended with type/id/name/description/auth/sockets.                                                                            | ⬜               | ⬜             | ⬜             |
| CLI-HW-17 | Corrupt/partial `devices.json`.                        | The CLI logs an error and starts with a usable (empty) database instead of crashing.                                                   | ⬜               | ⬜             | ⬜             |
| CLI-HW-18 | Gembird SIS-PM: plug in; `list`; toggle each of 4 sockets. | Detected as `Gembird SIS-PM`; 4 sockets switch physically; status reads back.                                                         | ⬜               | ⬜             | ⬜             |
| CLI-HW-19 | Gembird MSIS-PM: plug in; toggle the single socket.    | Detected as `Gembird MSIS-PM`; 1 socket switches.                                                                                      | ⬜               | ⬜             | ⬜             |
| CLI-HW-20 | Gembird MSIS-PM (2): plug in; toggle the single socket. | Detected as `Gembird MSIS-PM (2)`; 1 socket switches.                                                                                  | ⬜               | ⬜             | ⬜             |
| CLI-HW-21 | Energenie EG-PMS: plug in; toggle each of 4 sockets.   | Detected as `Energenie EG-PMS`; serial read from device; 4 sockets switch.                                                             | ⬜               | ⬜             | ⬜             |
| CLI-HW-22 | Energenie EG-PMS2: plug in; toggle each of 4 sockets.  | Detected as `Energenie EG-PMS2`; 4 sockets switch.                                                                                     | ⬜               | ⬜             | ⬜             |
| CLI-HW-23 | EG-PMxx-LAN: discover on LAN; address by serial; toggle each of 4 sockets; read status. | Detected via MAC/port; addressable by serial; 4 sockets switch; status echoed back.                          | ⬜               | ⬜             | ⬜             |
| CLI-HW-24 | Any USB: unplug the device mid-session.                | Device shown as disconnected; operations fail gracefully; no crash.                                                                    | ⬜               | ⬜             | ⬜             |
| CLI-HW-25 | Any LAN: unplug the device mid-session.                | Device shown as disconnected; operations fail gracefully; no crash.                                                                    | ⬜               | ⬜             | ⬜             |
| CLI-HW-26 | Linux, any USB: run without udev rules, then with them. | Without rules: access denied/enumeration fails; with rules: works.                                                                    | ⚠️               | ⬜             | ⚠️             |

## B. Graphical interface (`sokketter-ui`)

The cases are split into two groups: **functional** (`UI-FUN-*`) — everything that runs with **no
device present** (window chrome, empty list, Settings, About, theme); and **hardware** (`UI-HW-*`) —
cases that require at least one device real to be present.

### B1. Functional (no device required)

| ID        | Steps                                                        | Expected                                                                     | Result: Windows | Result: Linux | Result: macOS |
| --------- | ------------------------------------------------------------ | ---------------------------------------------------------------------------- | --------------- | ------------- | ------------- |
| UI-FUN-01 | Launch the app.                                              | Main window opens at the persisted geometry.                                 | ⬜               | ⬜             | ⬜             |
| UI-FUN-02 | With no devices present.                                     | The empty-list placeholder item is shown; clicking it does nothing.          | ⬜               | ⬜             | ⬜             |
| UI-FUN-03 | Click the refresh control.                                   | Device list re-enumerates.                                                   | ⬜               | ⬜             | ⬜             |
| UI-FUN-04 | Resize the window.                                           | List items reflow to the available width without clipping.                   | ⬜               | ⬜             | ⬜             |
| UI-FUN-05 | Open Settings; click "open data folder".                     | Storage folder opens in the OS file manager.                                 | ⬜               | ⬜             | ⬜             |
| UI-FUN-06 | Change theme radio (Auto/Light/Dark).                        | Whole app (incl. dialogs, list items) restyles live.                         | ⬜               | ⬜             | ⬜             |
| UI-FUN-07 | With theme = Auto, change the OS light/dark mode.            | App follows the OS theme; Windows title bar switches; macOS detection fires. | ⬜               | ⬜             | ⬜             |
| UI-FUN-08 | Open About.                                                  | Shows version, git hash, build date, license line, and components list.      | ⬜               | ⬜             | ⬜             |
| UI-FUN-09 | Open the License dialog; switch tabs.                        | Dialog opens, tabs render, OK closes it; respects the current theme.         | ⬜               | ⬜             | ⬜             |
| UI-FUN-10 | Change UI settings, move/resize the window, close, relaunch. | Geometry restored from `sokketter-ui.json`.                                  | ⬜               | ⬜             | ⬜             |

### B2. Hardware (device required)

| ID        | Steps                                                                                                          | Expected                                                                                              | Result: Windows | Result: Linux | Result: macOS |
| --------- | -------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------- | --------------- | ------------- | ------------- |
| UI-HW-01  | Toggle **USB devices** / **Ethernet devices** filters in Settings.                                             | List updates to include only the allowed bus types.                                                   | ⬜               | ⬜             | ⬜             |
| UI-HW-02  | Observe a disconnected saved device.                                                                           | Item shows disconnected state and is not interactive for switching.                                   | ⬜               | ⬜             | ⬜             |
| UI-HW-03  | Click a device with **no** auth (USB).                                                                         | Goes straight to the socket list.                                                                     | ⬜               | ⬜             | ⬜             |
| UI-HW-04  | Click an EG-PMxx-LAN device (password auth).                                                                   | Redirected to the password page.                                                                      | ⬜               | ⬜             | ⬜             |
| UI-HW-05  | Use the back control on the auth page.                                                                         | Returns to the device list.                                                                           | ⬜               | ⬜             | ⬜             |
| UI-HW-06  | Enter a wrong password.                                                                                        | `Authentication failed! Please try again.`; stays on the page.                                        | ⬜               | ⬜             | ⬜             |
| UI-HW-07  | Enter the correct password and log in (button or Enter).                                                       | `Authentication succeed!`; socket list opens; password saved.                                         | ⬜               | ⬜             | ⬜             |
| UI-HW-08  | After authenticating for the first time, visit the device page, go back and open the same device               | Entering password once again is not required                                                          | ⬜               | ⬜             | ⬜             |
| UI-HW-09  | Click a **connected** device with no auth or valid saved credentials.                                          | `try_authenticate()` runs on the worker thread; on success the socket list opens.                     | ⬜               | ⬜             | ⬜             |
| UI-HW-10  | Click a saved but **offline/disconnected** device (no auth or valid saved credentials).                        | Skips authentication and proceeds straight to the socket list; no connection attempt is forced.       | ⬜               | ⬜             | ⬜             |
| UI-HW-11  | Click a saved but **offline/disconnected** device (with auth but without valid saved credentials).             | Sockets are disabled (cannot toggle while offline), but configuration is still editable and persists. | ⬜               | ⬜             | ⬜             |
| UI-HW-12  | On an **offline** device's socket list, open the edit → Configure page, change device/socket fields, and Save. | Sockets are disabled (cannot toggle while offline), but configuration is still editable and persists. | ⬜               | ⬜             | ⬜             |
| UI-HW-13  | Reconnect a device whose saved password no longer works.                                                       | Auto-redirect back to the authentication page.                                                        | ⬜               | ⬜             | ⬜             |
| UI-HW-14  | In **single-click** mode, click a socket.                                                                      | Socket toggles; status LED updates after the async operation.                                         | ⬜               | ⬜             | ⬜             |
| UI-HW-15  | For a socket with a configurable reset, press reset.                                                           | Socket powers off, waits the configured ms, powers back on; button disabled during reset.             | ⬜               | ⬜             | ⬜             |
| UI-HW-16  | Proceed to LAN device settings, change anything, save and go back to socket list.                              | Socket are having the same status as before (ON or OFF) and not UNKNOWN                               | ⬜               | ⬜             | ⬜             |
| UI-HW-17  | Socket with reset = 0 ms.                                                                                      | Reset button is hidden.                                                                               | ⬜               | ⬜             | ⬜             |
| UI-HW-18  | Rapidly toggle multiple sockets.                                                                               | UI stays responsive (I/O on worker thread); no stuck/disabled sockets.                                | ⬜               | ⬜             | ⬜             |
| UI-HW-19  | Switch to **double-click** mode in Settings, single-click a socket.                                            | Single click does nothing; double click toggles.                                                      | ⬜               | ⬜             | ⬜             |
| UI-HW-20  | Open the socket list edit → Configure page.                                                                    | Device edit form + one form per socket are shown.                                                     | ⬜               | ⬜             | ⬜             |
| UI-HW-21  | Use Configure/back controls.                                                                                   | Navigation returns without saving.                                                                    | ⬜               | ⬜             | ⬜             |
| UI-HW-22  | Change device name/description and Save.                                                                       | New values persist and appear in the device/socket lists after restart.                               | ⬜               | ⬜             | ⬜             |
| UI-HW-23  | Change socket name/description and Save.                                                                       | New values persist.                                                                                   | ⬜               | ⬜             | ⬜             |
| UI-HW-24  | Set a socket configurable-reset value (validated integer).                                                     | Only non-negative integers accepted; reset button appears when > 0.                                   | ⬜               | ⬜             | ⬜             |
| UI-HW-25  | Delete/forget the device, choose **No**.                                                                       | Nothing is deleted.                                                                                   | ⬜               | ⬜             | ⬜             |
| UI-HW-26  | Delete/forget the device, confirm the warning dialog with **Yes**.                                             | Device removed from list and `devices.json`; returns to device list.                                  | ⬜               | ⬜             | ⬜             |
| UI-HW-27  | Edit any config in application, restart the app.                                                               | Edited values survive the restart.                                                                    | ⬜               | ⬜             | ⬜             |
| UI-HW-28  | Load an old `devices.json` with no `authentication-type`.                                                      | Loads without error; auth type defaults from the device class (backwards compatible).                 | ⬜               | ⬜             | ⬜             |
| UI-HW-29  | Gembird SIS-PM: plug in; open the device; toggle each of 4 sockets.                                           | Detected as `Gembird SIS-PM`; 4 sockets switch physically; status reads back.                         | ⬜               | ⬜             | ⬜             |
| UI-HW-30  | Gembird MSIS-PM: plug in; toggle the single socket.                                                            | Detected as `Gembird MSIS-PM`; 1 socket switches.                                                     | ⬜               | ⬜             | ⬜             |
| UI-HW-31  | Gembird MSIS-PM (2): plug in; toggle the single socket.                                                        | Detected as `Gembird MSIS-PM (2)`; 1 socket switches.                                                 | ⬜               | ⬜             | ⬜             |
| UI-HW-32  | Energenie EG-PMS: plug in; toggle each of 4 sockets.                                                           | Detected as `Energenie EG-PMS`; serial read from device; 4 sockets switch.                            | ⬜               | ⬜             | ⬜             |
| UI-HW-33  | Energenie EG-PMS2: plug in; toggle each of 4 sockets.                                                          | Detected as `Energenie EG-PMS2`; 4 sockets switch.                                                    | ⬜               | ⬜             | ⬜             |
| UI-HW-34  | EG-PMxx-LAN: discover on LAN; authenticate; toggle each of 4 sockets; read status.                             | Detected via MAC/port; password login works; 4 sockets switch; status echoed back.                    | ⬜               | ⬜             | ⬜             |
| UI-HW-35  | Any USB: unplug the device mid-session.                                                                        | Device shown as disconnected; operations fail gracefully; no crash.                                   | ⬜               | ⬜             | ⬜             |
| UI-HW-36  | Any LAN: unplug the device mid-session.                                                                        | Device shown as disconnected; operations fail gracefully; no crash.                                   | ⬜               | ⬜             | ⬜             |
| UI-HW-37  | Linux, any USB: run without udev rules, then with them.                                                        | Without rules: access denied/enumeration fails; with rules: works.                                    | ⚠️               | ⬜             | ⚠️             |

---

# Future development

Test-case IDs are prefixed `AUTO-`.

## Current state

| Suite            | Location                                               | Status                                                                                                                                                                                                                                                     |
| ---------------- | ------------------------------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| CLI parser tests | [sokketter-cli/tests](sokketter-cli/tests)             | Present — options, subcommands, general parsing, plus full `CLI-PRS-01`…`CLI-PRS-20` / `CLI-HW-01`…`CLI-HW-15` coverage (short flags, mixed case, windows-style, underscore aliasing, dedup, validate-before-hardware, actions via serial, multi-socket toggle, conversion errors). |
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

Widget-level tests with `QtTest`.

| ID | Under test | Derived from | Notes                                                                                                                                                    |
| ---------- | ---------------------------------------------- | --------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| AUTO-UI-01 | Device list population + empty placeholder | UI-FUN-01/02 | Assert item widgets after enumeration.                                                                                                                   |
| AUTO-UI-02 | Bus filter behavior | UI-HW-01 | Toggle checkboxes → `repopulate_device_list`.                                                                                                            |
| AUTO-UI-03 | Socket toggle single vs double click | UI-HW-14/19 | `QTest::mouseClick` / `mouseDClick`; `QSignalSpy` on state change.                                                                                       |
| AUTO-UI-04 | Configurable-reset flow | UI-HW-15/17 | `toggleResetButton` signal; button hidden at 0 ms.                                                                                                       |
| AUTO-UI-05 | `DeviceEditForm` / `SocketEditForm` round-trip | UI-HW-20/22/24 | Set fields → `configuration()` returns them; integer validator enforced.                                                                                 |
| AUTO-UI-06 | Settings persistence | UI-FUN-10 | `app_settings_storage` round-trip for toggle/theme/geometry/filters.                                                                                     |
| AUTO-UI-07 | Theme switch | UI-FUN-06 | Stylesheet changes on `ThemeChange`.                                                                                                                     |
| AUTO-UI-08 | Auth page routing | UI-HW-03/04 | No-auth → socket list; password device → auth page.                                                                                                      |
| AUTO-UI-09 | Offline-device click routing | UI-HW-10/11 | Offline saved device → socket list without `try_authenticate()`; connected device with failing auth → auth page. Needs a saved-but-disconnected fixture. |
