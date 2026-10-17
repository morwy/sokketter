#!/usr/bin/env python

"""
CLI test runner script for the project.

Runs the sokketter-cli test cases defined in TESTING.md (section A: parsing cases CLI-PRS-01..20
that require no hardware, and hardware cases CLI-HW-01..26 that require real devices) directly
against a built sokketter-cli binary. Every case is executed as a scripted invocation with
exit-code, stdout and stderr assertions; cases that are inherently physical (plugging in a
specific device, unplugging mid-session) prompt the operator for a Pass/Fail/Skip verdict.

The script expects a path to the folder containing the sokketter-cli binary. The path can be
provided via the --folder flag; without the flag the user is prompted for it. If nothing valid
is provided, the script exits with an error.

The run is split into two phases:
1. Parsing phase - all cases that work without any hardware. The operator is asked to disconnect
   all hardware beforehand, and the sokketter device database is cleaned for a fresh start.
2. Hardware phase - all cases that require real hardware. The operator is asked to switch the
   hardware on beforehand, and the device database is cleaned again.

At the end a results table with one row per test case (test id and status) is printed.

This script is designed to be cross-platform and should work on Windows, Linux, and macOS.
"""

# --------------------------------------------------------------------------------------------------
#
# Imports.
#
# --------------------------------------------------------------------------------------------------
import argparse
import json
import logging
import os
import re
import subprocess
import sys
from enum import Enum

from Environment import Environment, System


# --------------------------------------------------------------------------------------------------
#
# Global variables.
#
# --------------------------------------------------------------------------------------------------
class TestStatus(str, Enum):
    """
    Enum to define the possible results of a single test case.
    """

    PASS = "Pass"
    FAIL = "Fail"
    BLOCKED = "Blocked"
    SKIPPED = "Skipped"


class CaseFailure(Exception):
    """
    Raised when a test case does not match its expectation; the case is recorded as Fail.
    """


class CaseBlocked(Exception):
    """
    Raised when a test case cannot be executed on the current setup; it is recorded as Blocked.
    """


# Device type display strings produced by power_strip::to_string(), used to parse list output.
DEVICE_TYPE_PATTERN = (
    r"Gembird SIS-PM|Gembird MSIS-PM \(2\)|Gembird MSIS-PM|Energenie EG-PMS|"
    r"Energenie EG-PMS2|Energenie EG-PMxx-LAN|TEST DEVICE|Unknown"
)

DEVICE_LINE_REGEX = re.compile(
    r"^(?P<index>\d+)\. .*"
    r"\((?P<type>" + DEVICE_TYPE_PATTERN + r"), (?P<id>[^,]+), "
    r"(?P<address>available at .+|disconnected)\)$"
)

LAN_TYPE = "Energenie EG-PMxx-LAN"


# --------------------------------------------------------------------------------------------------
#
# Class definition.
#
# --------------------------------------------------------------------------------------------------
class CliTests:
    """
    Class to run the TESTING.md CLI cases against a built sokketter-cli binary.
    """

    def __init__(
        self, folder: str, storage_path: str | None, assume_hardware: bool
    ) -> None:
        """
        Initialize the CLI tests class.
        """
        self.logger = logging.getLogger(__name__)
        self.logger.info("CliTests class initialized.")

        self.system = Environment.get_os()
        self.logger.info("Operating system: %s", self.system.value)

        self.cli_binary = self.__resolve_cli_binary(folder)
        self.logger.info("Target sokketter-cli binary: %s", self.cli_binary)

        self.storage_path = storage_path or self.__get_default_storage_path()
        self.logger.info("sokketter storage path: %s", self.storage_path)

        self.assume_hardware = assume_hardware
        self.logger.info("Assume hardware ready: %s", self.assume_hardware)

        self.results: list[tuple[str, TestStatus]] = []

        self.usb_inventory: list[dict[str, str]] = []
        self.lan_inventory: list[dict[str, str]] = []

    # ---------------------------------------------------------------------------------------------
    #
    # Supporting functions.
    #
    # ---------------------------------------------------------------------------------------------
    def __resolve_cli_binary(self, folder: str) -> str:
        """
        Validate the given folder and resolve the sokketter-cli executable inside it.
        """
        path = os.path.abspath(os.path.expanduser(folder))

        if os.path.isfile(path):
            return path

        if not os.path.isdir(path):
            raise FileNotFoundError(f"Provided folder does not exist: {path}")

        executable_name = (
            "sokketter-cli.exe" if self.system == System.WINDOWS else "sokketter-cli"
        )
        candidate = os.path.join(path, executable_name)

        if not os.path.isfile(candidate):
            raise FileNotFoundError(
                f"sokketter-cli binary was not found in folder: {path}"
            )

        return candidate

    def __get_default_storage_path(self) -> str:
        """
        Get the default sokketter storage path for the current platform.
        """
        if self.system == System.WINDOWS:
            program_data = os.environ.get("ProgramData", "C:\\ProgramData")
            return os.path.join(program_data, "sokketter")

        if self.system == System.MACOS:
            return "/Users/Shared/sokketter"

        if self.system == System.LINUX:
            return os.path.join(os.path.expanduser("~"), ".local", "share", "sokketter")

        raise EnvironmentError("Unsupported platform")

    def __clean_device_database(self) -> None:
        """
        Delete the device database (devices.json) to start every phase from a clean slate.
        """
        database_file = os.path.join(self.storage_path, "devices.json")

        if os.path.exists(database_file):
            os.remove(database_file)
            self.logger.info("Removed device database file: %s", database_file)
        else:
            self.logger.info("No device database file to remove: %s", database_file)

    def __build_environment(self, test_device_number: int | None) -> dict[str, str]:
        """
        Build the subprocess environment for a CLI invocation.

        @attention test_device_number forces an artificial device bus via the
        LIBSOKKETTER_TEST_DEVICE_NUMBER variable; passing None leaves real devices enumerable.
        @attention the update check is always skipped to keep the run deterministic and offline.
        """
        environment = os.environ.copy()
        environment["LIBSOKKETTER_TEST_SKIP_UPDATE_CHECK"] = "1"

        if test_device_number is not None:
            environment["LIBSOKKETTER_TEST_DEVICE_NUMBER"] = str(test_device_number)
        else:
            environment.pop("LIBSOKKETTER_TEST_DEVICE_NUMBER", None)

        return environment

    def __run_cli(
        self, arguments: list[str], test_device_number: int | None = 0
    ) -> subprocess.CompletedProcess:
        """
        Run the sokketter-cli binary with the given arguments and capture its output.
        """
        command = [self.cli_binary] + arguments
        self.logger.info("Executing: %s", " ".join(command))

        result = subprocess.run(
            command,
            check=False,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            env=self.__build_environment(test_device_number),
            timeout=120,
        )

        self.logger.info("Exit code: %d", result.returncode)
        return result

    @staticmethod
    def __expect_exit(
        result: subprocess.CompletedProcess, expected: int, context: str
    ) -> None:
        """
        Assert the documented exit-code contract (0 success, 1 any failure).
        """
        if result.returncode != expected:
            raise CaseFailure(
                f"{context}: expected exit code {expected}, got {result.returncode}; "
                f"stdout: {result.stdout!r}; stderr: {result.stderr!r}"
            )

    @staticmethod
    def __expect_in(haystack: str, needle: str, context: str) -> None:
        """
        Assert that the given text appears in the captured output.
        """
        if needle not in haystack:
            raise CaseFailure(
                f"{context}: expected {needle!r} in output, got {haystack!r}"
            )

    @staticmethod
    def __expect_not_in(haystack: str, needle: str, context: str) -> None:
        """
        Assert that the given text does not appear in the captured output.
        """
        if needle in haystack:
            raise CaseFailure(
                f"{context}: did not expect {needle!r} in output, got {haystack!r}"
            )

    @staticmethod
    def __expect_match(pattern: str, text: str, context: str) -> re.Match:
        """
        Assert that the given regular expression matches somewhere in the captured output.
        """
        match = re.search(pattern, text)
        if match is None:
            raise CaseFailure(
                f"{context}: expected pattern {pattern!r} in output, got {text!r}"
            )

        return match

    @staticmethod
    def __parse_list_output(
        result: subprocess.CompletedProcess,
    ) -> list[dict[str, str]]:
        """
        Parse the numbered device lines printed by the list subcommand.
        """
        devices: list[dict[str, str]] = []

        for line in result.stdout.splitlines():
            match = DEVICE_LINE_REGEX.match(line.strip())
            if match is not None:
                devices.append(
                    {
                        "index": match.group("index"),
                        "type": match.group("type"),
                        "id": match.group("id"),
                        "address": match.group("address"),
                    }
                )

        return devices

    def __enumerate_devices(self) -> None:
        """
        Populate the USB and Ethernet inventories by enumerating real hardware.
        """
        usb_result = self.__run_cli(["list"], test_device_number=None)
        self.__expect_exit(usb_result, 0, "USB enumeration")
        self.usb_inventory = self.__parse_list_output(usb_result)

        lan_result = self.__run_cli(
            ["list", "--include-device-types", "ethernet"], test_device_number=None
        )
        self.__expect_exit(lan_result, 0, "Ethernet enumeration")
        self.lan_inventory = self.__parse_list_output(lan_result)

        self.logger.info(
            "Enumerated %d USB and %d Ethernet device(s).",
            len(self.usb_inventory),
            len(self.lan_inventory),
        )

    def __require_first_usb(self) -> dict[str, str]:
        """
        Return the first USB device from the inventory or block the calling case.
        """
        if not self.usb_inventory:
            raise CaseBlocked("no USB device is connected")

        return self.usb_inventory[0]

    def __require_first_lan(self) -> dict[str, str]:
        """
        Return the first Ethernet device from the inventory or block the calling case.
        """
        if not self.lan_inventory:
            raise CaseBlocked("no Ethernet device is connected")

        return self.lan_inventory[0]

    def __socket_count_of_first_usb(self) -> int:
        """
        Determine the socket count of the first USB device from its status output.
        """
        result = self.__run_cli(["power", "status", "-i", "1"], test_device_number=None)
        self.__expect_exit(result, 0, "socket count lookup")

        return len(re.findall(r"^\s+Socket \d+:", result.stdout, re.MULTILINE))

    @staticmethod
    def __count_occurrences(text: str, needle: str) -> int:
        """
        Count how many times the given line text occurs in the output.
        """
        return sum(1 for line in text.splitlines() if needle in line)

    def __read_device_database(self) -> list:
        """
        Read and parse devices.json from the storage path; return the parsed device list.
        """
        database_file = os.path.join(self.storage_path, "devices.json")

        if not os.path.exists(database_file):
            raise CaseFailure(f"device database file was not created: {database_file}")

        try:
            with open(file=database_file, mode="r", encoding="utf-8") as file:
                database = json.load(file)
        except json.JSONDecodeError as error:
            raise CaseFailure(f"device database file is not valid JSON: {error}")

        if not isinstance(database, list):
            raise CaseFailure(
                f"device database file does not contain a list: {database!r}"
            )

        return database

    def __prompt(self, message: str) -> None:
        """
        Ask the operator to confirm a hardware state change before continuing.
        """
        if self.assume_hardware:
            self.logger.info("Assuming hardware state; skipping prompt: %s", message)
            return

        try:
            input(f"\n{message}\nPress Enter when ready... ")
        except EOFError:
            self.logger.info(
                "No interactive terminal; continuing without confirmation."
            )

    def __manual_verdict(self, case_id: str, steps: str, expected: str) -> TestStatus:
        """
        Present the operator with the TESTING.md steps of a physical case and ask for a verdict.
        """
        self.logger.info("%s is a manual case; asking the operator.", case_id)
        print(f"\n--- {case_id} ---\nSteps: {steps}\nExpected: {expected}\n")

        while True:
            try:
                answer = (
                    input(f"Result for {case_id} ([P]ass / [F]ail / [S]kip)? ")
                    .strip()
                    .lower()
                )
            except EOFError:
                self.logger.info(
                    "No interactive terminal; recording %s as skipped.", case_id
                )
                return TestStatus.SKIPPED

            if answer in ("p", "pass"):
                return TestStatus.PASS
            if answer in ("f", "fail"):
                return TestStatus.FAIL
            if answer in ("s", "skip"):
                return TestStatus.SKIPPED

            print("Please answer P, F or S.")

    def __record(self, case_id: str, status: TestStatus, detail: str = "") -> None:
        """
        Store the result of one test case and log it.
        """
        self.results.append((case_id, status))

        if detail:
            self.logger.info("%s: %s (%s)", case_id, status.value, detail)
        else:
            self.logger.info("%s: %s", case_id, status.value)

    def __run_case(self, case_id: str, test_function) -> None:
        """
        Execute one test function and translate its outcome into a recorded status.
        """
        self.logger.info("Running test case %s.", case_id)

        try:
            returned_status = test_function()
            self.__record(case_id, returned_status or TestStatus.PASS)

        except CaseBlocked as error:
            self.__record(case_id, TestStatus.BLOCKED, str(error))

        except CaseFailure as error:
            self.__record(case_id, TestStatus.FAIL, str(error))

        except (
            Exception
        ) as error:  # noqa: BLE001 - a case crash must not abort the suite.
            self.__record(case_id, TestStatus.FAIL, f"unexpected error: {error}")

    # ---------------------------------------------------------------------------------------------
    #
    # No-hardware test functions (CLI-PRS-*). One function per test case from TESTING.md.
    #
    # ---------------------------------------------------------------------------------------------
    def test_cli_prs_01(self) -> None:
        """
        @brief CLI-PRS-01: no args. Exit 1; stderr "A subcommand is required".
        """
        result = self.__run_cli([])
        self.__expect_exit(result, 1, "no args")
        self.__expect_in(result.stderr, "A subcommand is required", "no args")

    def test_cli_prs_02(self) -> None:
        """
        @brief CLI-PRS-02: --help / -h. Exit 0; prints the descriptive help text.
        """
        for flag in ("--help", "-h"):
            result = self.__run_cli([flag])
            self.__expect_exit(result, 0, f"{flag} help")
            self.__expect_in(result.stdout, "sokketter-cli", f"{flag} help")

    def test_cli_prs_03(self) -> None:
        """
        @brief CLI-PRS-03: --version / -v. Exit 0; prints "sokketter-cli version ...".
        """
        for flag in ("--version", "-v"):
            result = self.__run_cli([flag])
            self.__expect_exit(result, 0, f"{flag} version")
            self.__expect_in(result.stdout, "sokketter-cli version", f"{flag} version")

    def test_cli_prs_04(self) -> None:
        """
        @brief CLI-PRS-04: list with no devices. Exit 0; stdout listing header then
        "No devices found." (message, not exit code, marks the empty bus).
        """
        result = self.__run_cli(["list"])
        self.__expect_exit(result, 0, "list no devices")
        self.__expect_in(result.stdout, "Listing available devices", "list no devices")
        self.__expect_in(result.stdout, "No devices found.", "list no devices")

    def test_cli_prs_05(self) -> None:
        """
        @brief CLI-PRS-05: "list power" / "power list". Exit 1; unexpected argument error.
        """
        for arguments in (["list", "power"], ["power", "list"]):
            result = self.__run_cli(arguments)
            self.__expect_exit(result, 1, " ".join(arguments))
            self.__expect_in(
                result.stderr,
                "The following argument was not expected",
                " ".join(arguments),
            )

    def test_cli_prs_06(self) -> None:
        """
        @brief CLI-PRS-06: "power status -i 1 -n <serial>". Exit 1; the two device options exclude.
        """
        result = self.__run_cli(["power", "status", "-i", "1", "-n", "TEST-SERIAL"])
        self.__expect_exit(result, 1, "index excludes serial")
        self.__expect_in(
            result.stderr,
            "--device-at-index excludes --device-with-serial",
            "index excludes serial",
        )

    def test_cli_prs_07(self) -> None:
        """
        @brief CLI-PRS-07: "power status" without device flag. Exit 1; option group required.
        """
        result = self.__run_cli(["power", "status"])
        self.__expect_exit(result, 1, "power status without flags")
        self.__expect_in(
            result.stderr,
            "[Option Group: --device-at-index or --device-with-serial] is required.",
            "power status without flags",
        )

    def test_cli_prs_08(self) -> None:
        """
        @brief CLI-PRS-08: "power status -n <missing>". Exit 1; no device for that serial.
        """
        result = self.__run_cli(["power", "status", "-n", "MISSING-SERIAL"])
        self.__expect_exit(result, 1, "missing serial")
        self.__expect_in(
            result.stderr,
            'No device was found for serial "MISSING-SERIAL".',
            "missing serial",
        )

    def test_cli_prs_09(self) -> None:
        """
        @brief CLI-PRS-09: "power on/status/off/toggle --help". Exit 0; help takes precedence.
        """
        for action in ("on", "status", "off", "toggle"):
            result = self.__run_cli(["power", action, "--help"])
            self.__expect_exit(result, 0, f"power {action} --help")
            if not result.stdout.strip():
                raise CaseFailure(f"power {action} --help: help output is empty")

    def test_cli_prs_10(self) -> None:
        """
        @brief CLI-PRS-10: Windows-style /help, mixed case LIST, underscore aliases parse the
        same as their canonical forms.
        """
        result = self.__run_cli(["/help"])
        self.__expect_exit(result, 0, "windows-style /help")

        result = self.__run_cli(["LIST"])
        self.__expect_exit(result, 0, "mixed case LIST")
        self.__expect_in(result.stdout, "Listing available devices", "mixed case LIST")

        result = self.__run_cli(["power", "status", "--device_at_index", "1"])
        self.__expect_exit(result, 1, "underscore option alias")
        self.__expect_in(result.stderr, "is out of range", "underscore option alias")

    def test_cli_prs_11(self) -> None:
        """
        @brief CLI-PRS-11: "list --include-device-types ethernet". Exit 0; filter accepted.
        """
        result = self.__run_cli(["list", "--include-device-types", "ethernet"])
        self.__expect_exit(result, 0, "ethernet include filter")
        self.__expect_in(
            result.stdout, "Listing available devices", "ethernet include filter"
        )

    def test_cli_prs_12(self) -> None:
        """
        @brief CLI-PRS-12: "list --include-device-types lan". Exit 0; alias of ethernet.
        """
        result = self.__run_cli(["list", "--include-device-types", "lan"])
        self.__expect_exit(result, 0, "lan alias filter")
        self.__expect_in(result.stdout, "Listing available devices", "lan alias filter")

    def test_cli_prs_13(self) -> None:
        """
        @brief CLI-PRS-13: "power status -i 0". Exit 1; index 0 is out of range.
        """
        result = self.__run_cli(["power", "status", "-i", "0"])
        self.__expect_exit(result, 1, "index 0")
        self.__expect_in(
            result.stderr,
            "Device index 0 is out of range (valid range starts from 1).",
            "index 0",
        )

    def test_cli_prs_14(self) -> None:
        """
        @brief CLI-PRS-14: "power status -i 1 -t garbage". Exit 1; unknown device type error,
        and no update-notification line (validation runs before the update check).
        """
        result = self.__run_cli(["power", "status", "-i", "1", "-t", "garbage"])
        self.__expect_exit(result, 1, "unknown device type")
        self.__expect_in(
            result.stderr,
            "Unknown device type: garbage. Available types are: USB, ETHERNET, LAN.",
            "unknown device type",
        )
        self.__expect_not_in(
            result.stderr + result.stdout,
            "A new sokketter version",
            "unknown device type",
        )

    def test_cli_prs_15(self) -> None:
        """
        @brief CLI-PRS-15: "list --include-device-types ,usb" (and trailing comma). Exit 1;
        empty device type token error.
        """
        for value in (",usb", "usb,"):
            result = self.__run_cli(["list", "--include-device-types", value])
            self.__expect_exit(result, 1, f"empty device type token {value!r}")
            self.__expect_in(
                result.stderr,
                "Empty device type in --include-device-types.",
                f"empty device type token {value!r}",
            )

    def test_cli_prs_16(self) -> None:
        """
        @brief CLI-PRS-16: 'list --include-device-types " "'. Exit 1; whitespace-only token.
        """
        result = self.__run_cli(["list", "--include-device-types", " "])
        self.__expect_exit(result, 1, "whitespace device type")
        self.__expect_in(
            result.stderr,
            "Empty device type in --include-device-types.",
            "whitespace device type",
        )

    def test_cli_prs_17(self) -> None:
        """
        @brief CLI-PRS-17: "list --include-device-types=usb". Exit 0; = form accepted.
        """
        result = self.__run_cli(["list", "--include-device-types=usb"])
        self.__expect_exit(result, 0, "equals form filter")
        self.__expect_in(
            result.stdout, "Listing available devices", "equals form filter"
        )

    def test_cli_prs_18(self) -> None:
        """
        @brief CLI-PRS-18: "power status -i 1" with no devices. Exit 1; no devices available.
        """
        result = self.__run_cli(["power", "status", "-i", "1"])
        self.__expect_exit(result, 1, "index with no devices")
        self.__expect_in(
            result.stderr,
            "Device index 1 is out of range (no devices are available).",
            "index with no devices",
        )

    def test_cli_prs_19(self) -> None:
        """
        @brief CLI-PRS-19: SIZE_MAX parses and is out of range; SIZE_MAX + 1 is a conversion
        error. Both exit 1.
        """
        size_max_result = self.__run_cli(
            ["power", "status", "-i", "18446744073709551615"]
        )
        self.__expect_exit(size_max_result, 1, "SIZE_MAX index")
        self.__expect_in(size_max_result.stderr, "out of range", "SIZE_MAX index")

        overflow_result = self.__run_cli(
            ["power", "status", "-i", "18446744073709551616"]
        )
        self.__expect_exit(overflow_result, 1, "SIZE_MAX + 1 index")
        self.__expect_in(
            overflow_result.stderr, "18446744073709551616", "SIZE_MAX + 1 index"
        )

    def test_cli_prs_20(self) -> None:
        """
        @brief CLI-PRS-20: "list list" / "power status status". Exit 1; unexpected argument.
        """
        for arguments in (["list", "list"], ["power", "status", "status"]):
            result = self.__run_cli(arguments)
            self.__expect_exit(result, 1, " ".join(arguments))
            self.__expect_in(
                result.stderr,
                "The following argument was not expected",
                " ".join(arguments),
            )

    def run_no_hardware_phase(self) -> None:
        """
        Run the whole no-hardware (parsing) phase.
        """
        self.logger.info("Starting the no-hardware (parsing) phase.")

        self.__prompt(
            "PHASE 1: make sure ALL supported power-strip hardware is disconnected "
            "from this machine."
        )

        self.__clean_device_database()

        for case_id, test_function in [
            ("CLI-PRS-01", self.test_cli_prs_01),
            ("CLI-PRS-02", self.test_cli_prs_02),
            ("CLI-PRS-03", self.test_cli_prs_03),
            ("CLI-PRS-04", self.test_cli_prs_04),
            ("CLI-PRS-05", self.test_cli_prs_05),
            ("CLI-PRS-06", self.test_cli_prs_06),
            ("CLI-PRS-07", self.test_cli_prs_07),
            ("CLI-PRS-08", self.test_cli_prs_08),
            ("CLI-PRS-09", self.test_cli_prs_09),
            ("CLI-PRS-10", self.test_cli_prs_10),
            ("CLI-PRS-11", self.test_cli_prs_11),
            ("CLI-PRS-12", self.test_cli_prs_12),
            ("CLI-PRS-13", self.test_cli_prs_13),
            ("CLI-PRS-14", self.test_cli_prs_14),
            ("CLI-PRS-15", self.test_cli_prs_15),
            ("CLI-PRS-16", self.test_cli_prs_16),
            ("CLI-PRS-17", self.test_cli_prs_17),
            ("CLI-PRS-18", self.test_cli_prs_18),
            ("CLI-PRS-19", self.test_cli_prs_19),
            ("CLI-PRS-20", self.test_cli_prs_20),
        ]:
            self.__run_case(case_id, test_function)

        self.logger.info("No-hardware phase completed.")

    # ---------------------------------------------------------------------------------------------
    #
    # Hardware test functions (CLI-HW-*). One function per test case from TESTING.md.
    #
    # ---------------------------------------------------------------------------------------------
    def test_cli_hw_01(self) -> None:
        """
        @brief CLI-HW-01: "list" with N devices. Exit 0; numbered device list printed.
        """
        result = self.__run_cli(["list"], test_device_number=None)
        self.__expect_exit(result, 0, "list devices")

        if not self.usb_inventory:
            raise CaseBlocked("no USB device is connected")

        if len(self.usb_inventory) < 1:
            raise CaseFailure("list did not print any numbered device line")

    def test_cli_hw_02(self) -> None:
        """
        @brief CLI-HW-02: "power status -i 1 -s 1". Exit 0; device + socket 1 status printed.
        """
        self.__require_first_usb()
        result = self.__run_cli(
            ["power", "status", "-i", "1", "-s", "1"], test_device_number=None
        )
        self.__expect_exit(result, 0, "status by index")
        self.__expect_in(result.stdout, "Socket 1:", "status by index")

    def test_cli_hw_03(self) -> None:
        """
        @brief CLI-HW-03: "power status -n <serial> -s 1". Exit 0; same output by serial.
        """
        device = self.__require_first_usb()
        result = self.__run_cli(
            ["power", "status", "-n", device["id"], "-s", "1"], test_device_number=None
        )
        self.__expect_exit(result, 0, "status by serial")
        self.__expect_in(result.stdout, "Socket 1:", "status by serial")

    def test_cli_hw_04(self) -> None:
        """
        @brief CLI-HW-04: "power on -i 1" without --sockets. Exit 0; all sockets turned on.
        """
        self.__require_first_usb()
        expected_sockets = self.__socket_count_of_first_usb()
        result = self.__run_cli(["power", "on", "-i", "1"], test_device_number=None)
        self.__expect_exit(result, 0, "power on all sockets")

        turned_on = self.__count_occurrences(result.stdout, "turned on.")
        if turned_on != expected_sockets:
            raise CaseFailure(
                f"expected {expected_sockets} 'turned on.' lines, got {turned_on}: "
                f"{result.stdout!r}"
            )

    def test_cli_hw_05(self) -> None:
        """
        @brief CLI-HW-05: "power on -i 1 -s 1". Exit 0; only socket 1 turned on.
        """
        self.__require_first_usb()
        result = self.__run_cli(
            ["power", "on", "-i", "1", "-s", "1"], test_device_number=None
        )
        self.__expect_exit(result, 0, "power on socket 1")

        if self.__count_occurrences(result.stdout, "turned on.") != 1:
            raise CaseFailure(
                f"expected exactly one 'turned on.' line, got: {result.stdout!r}"
            )

    def test_cli_hw_06(self) -> None:
        """
        @brief CLI-HW-06: "power off -i 1 -s 1 2". Exit 0; sockets 1 and 2 turned off.
        """
        self.__require_first_usb()
        if self.__socket_count_of_first_usb() < 2:
            raise CaseBlocked("first USB device has fewer than 2 sockets")

        result = self.__run_cli(
            ["power", "off", "-i", "1", "-s", "1", "2"], test_device_number=None
        )
        self.__expect_exit(result, 0, "power off sockets 1 and 2")

        if self.__count_occurrences(result.stdout, "turned off.") != 2:
            raise CaseFailure(
                f"expected two 'turned off.' lines, got: {result.stdout!r}"
            )

    def test_cli_hw_07(self) -> None:
        """
        @brief CLI-HW-07: "power toggle -i 1 -s 1" twice. Exit 0; socket back to original state.
        """
        self.__require_first_usb()

        before = self.__run_cli(
            ["power", "status", "-i", "1", "-s", "1"], test_device_number=None
        )
        self.__expect_exit(before, 0, "toggle before")
        before_line = self.__expect_match(
            r"Socket 1: .*status: (on|off)", before.stdout, "toggle"
        )

        for _ in range(2):
            toggle = self.__run_cli(
                ["power", "toggle", "-i", "1", "-s", "1"], test_device_number=None
            )
            self.__expect_exit(toggle, 0, "toggle run")
            self.__expect_in(toggle.stdout, "toggled.", "toggle run")

        after = self.__run_cli(
            ["power", "status", "-i", "1", "-s", "1"], test_device_number=None
        )
        self.__expect_exit(after, 0, "toggle after")
        after_line = self.__expect_match(
            r"Socket 1: .*status: (on|off)", after.stdout, "toggle"
        )

        if before_line.group(0) != after_line.group(0):
            raise CaseFailure(
                f"socket state changed after two toggles: {before_line.group(0)!r} -> "
                f"{after_line.group(0)!r}"
            )

    def test_cli_hw_08(self) -> None:
        """
        @brief CLI-HW-08: "power status -i 1 -s 0" and "-s 99". Exit 1; socket out of range.
        """
        self.__require_first_usb()

        for socket_index in ("0", "99"):
            result = self.__run_cli(
                ["power", "status", "-i", "1", "-s", socket_index],
                test_device_number=None,
            )
            self.__expect_exit(result, 1, f"socket index {socket_index}")
            self.__expect_match(
                r"Socket index \d+ is out of range \(valid range: 1-\d+\)\.",
                result.stderr,
                f"socket index {socket_index}",
            )

    def test_cli_hw_09(self) -> None:
        """
        @brief CLI-HW-09: "power status -i 1 -t usb". Exit 0; index resolves under USB selection.
        """
        self.__require_first_usb()
        result = self.__run_cli(
            ["power", "status", "-i", "1", "-t", "usb"], test_device_number=None
        )
        self.__expect_exit(result, 0, "index under usb selection")
        self.__expect_in(result.stdout, "Socket 1:", "index under usb selection")

    def test_cli_hw_10(self) -> None:
        """
        @brief CLI-HW-10: "power status -i 1 -t ethernet" with a real Ethernet device online.
        Exit 0; the real Ethernet device stays addressable by index under an Ethernet selection.
        """
        self.__require_first_lan()
        result = self.__run_cli(
            ["power", "status", "-i", "1", "-t", "ethernet"], test_device_number=None
        )
        self.__expect_exit(result, 0, "index under ethernet selection")
        self.__expect_in(result.stdout, "Socket 1:", "index under ethernet selection")

    def test_cli_hw_11(self) -> None:
        """
        @brief CLI-HW-11: "power status -n <real Ethernet serial> -t ethernet". Exit 0; the real
        Ethernet device stays addressable by serial under an Ethernet selection.
        """
        device = self.__require_first_lan()
        result = self.__run_cli(
            ["power", "status", "-n", device["id"], "-t", "ethernet"],
            test_device_number=None,
        )
        self.__expect_exit(result, 0, "serial under ethernet selection")
        self.__expect_in(result.stdout, "Socket 1:", "serial under ethernet selection")

    def test_cli_hw_12(self) -> None:
        """
        @brief CLI-HW-12: "power status -n <real USB serial> -t ethernet". Exit 1; a serial
        outside the selection is reported as not found.
        """
        device = self.__require_first_usb()
        result = self.__run_cli(
            ["power", "status", "-n", device["id"], "-t", "ethernet"],
            test_device_number=None,
        )
        self.__expect_exit(result, 1, "USB serial under ethernet selection")
        self.__expect_in(
            result.stderr,
            f'No device was found for serial "{device["id"]}".',
            "USB serial under ethernet selection",
        )

    def test_cli_hw_13(self) -> None:
        """
        @brief CLI-HW-13: last valid socket index exits 0; one past it exits 1 with the range
        named in the message.
        """
        self.__require_first_usb()
        socket_count = self.__socket_count_of_first_usb()

        inside = self.__run_cli(
            ["power", "status", "-i", "1", "-s", str(socket_count)],
            test_device_number=None,
        )
        self.__expect_exit(inside, 0, f"socket index {socket_count}")

        outside = self.__run_cli(
            ["power", "status", "-i", "1", "-s", str(socket_count + 1)],
            test_device_number=None,
        )
        self.__expect_exit(outside, 1, f"socket index {socket_count + 1}")
        self.__expect_in(
            outside.stderr,
            f"valid range: 1-{socket_count}",
            f"socket index {socket_count + 1}",
        )

    def test_cli_hw_14(self) -> None:
        """
        @brief CLI-HW-14: "-i N" exits 0 while "-i N+1" exits 1 naming the valid range.
        """
        if not self.usb_inventory:
            raise CaseBlocked("no USB device is connected")

        device_count = len(self.usb_inventory)

        inside = self.__run_cli(
            ["power", "status", "-i", str(device_count)], test_device_number=None
        )
        self.__expect_exit(inside, 0, f"device index {device_count}")

        outside = self.__run_cli(
            ["power", "status", "-i", str(device_count + 1)], test_device_number=None
        )
        self.__expect_exit(outside, 1, f"device index {device_count + 1}")
        self.__expect_in(
            outside.stderr,
            f"valid range: 1-{device_count}",
            f"device index {device_count + 1}",
        )

    def test_cli_hw_15(self) -> None:
        """
        @brief CLI-HW-15: "power off -i 1 -s 2 1" and "-s 1 1 2 2". Exit 0; sockets reported in
        sorted order and repeated indices applied once.
        """
        self.__require_first_usb()
        if self.__socket_count_of_first_usb() < 2:
            raise CaseBlocked("first USB device has fewer than 2 sockets")

        for socket_arguments in (["-s", "2", "1"], ["-s", "1", "1", "2", "2"]):
            result = self.__run_cli(
                ["power", "off", "-i", "1"] + socket_arguments, test_device_number=None
            )
            self.__expect_exit(result, 0, "power off socket order")

            lines = [
                line for line in result.stdout.splitlines() if "turned off." in line
            ]
            if len(lines) != 2:
                raise CaseFailure(
                    f"expected exactly two 'turned off.' lines, got {lines!r}"
                )
            if "Socket 1:" not in lines[0] or "Socket 2:" not in lines[1]:
                raise CaseFailure(
                    f"sockets were not reported in sorted order: {lines!r}"
                )

    def test_cli_hw_16(self) -> None:
        """
        @brief CLI-HW-16: a connected device is appended to devices.json with
        type/id/name/description/auth/sockets fields.
        """
        if not self.usb_inventory and not self.lan_inventory:
            raise CaseBlocked("no device is connected to be persisted")

        database = self.__read_device_database()
        if not database:
            raise CaseFailure("device database is empty although hardware is connected")

        required_keys = {
            "type",
            "id",
            "name",
            "description",
            "authentication-type",
            "sockets",
        }

        for device in database:
            missing = required_keys - set(device.keys())
            if missing:
                raise CaseFailure(f"device entry is missing keys {missing}: {device!r}")
            if not isinstance(device["sockets"], list):
                raise CaseFailure(f"device sockets field is not a list: {device!r}")

    def test_cli_hw_17(self) -> None:
        """
        @brief CLI-HW-17: corrupt devices.json. The CLI logs an error and starts with a usable
        (empty) database instead of crashing.
        """
        database_file = os.path.join(self.storage_path, "devices.json")

        backup_file = database_file + ".bak"
        if os.path.exists(database_file):
            with open(file=database_file, mode="rb") as source:
                content = source.read()
            with open(file=backup_file, mode="wb") as target:
                target.write(content)

        with open(file=database_file, mode="w", encoding="utf-8") as file:
            file.write("{corrupted device database!!!")

        try:
            result = self.__run_cli(["list"], test_device_number=None)
            self.__expect_exit(result, 0, "corrupted database startup")
            self.__expect_in(
                result.stderr,
                "Starting with an empty database",
                "corrupted database startup",
            )
        finally:
            if os.path.exists(backup_file):
                os.replace(backup_file, database_file)
            elif os.path.exists(database_file):
                os.remove(database_file)

    def test_cli_hw_18(self) -> TestStatus:
        """
        @brief CLI-HW-18: Gembird SIS-PM plug-in; toggle each of 4 sockets. Physical case.
        """
        if not any(device["type"] == "Gembird SIS-PM" for device in self.usb_inventory):
            raise CaseBlocked("no Gembird SIS-PM device is connected")

        return self.__manual_verdict(
            "CLI-HW-18",
            "Plug in the Gembird SIS-PM; run 'list'; toggle each of the 4 sockets via "
            "'power toggle -i 1 -s N'.",
            "Detected as Gembird SIS-PM; 4 sockets switch physically; status reads back.",
        )

    def test_cli_hw_19(self) -> TestStatus:
        """
        @brief CLI-HW-19: Gembird MSIS-PM plug-in; toggle the single socket. Physical case.
        """
        if not any(
            device["type"] == "Gembird MSIS-PM" for device in self.usb_inventory
        ):
            raise CaseBlocked("no Gembird MSIS-PM device is connected")

        return self.__manual_verdict(
            "CLI-HW-19",
            "Plug in the Gembird MSIS-PM; run 'list'; toggle the single socket.",
            "Detected as Gembird MSIS-PM; 1 socket switches.",
        )

    def test_cli_hw_20(self) -> TestStatus:
        """
        @brief CLI-HW-20: Gembird MSIS-PM (2) plug-in; toggle the single socket. Physical case.
        """
        if not any(
            device["type"] == "Gembird MSIS-PM (2)" for device in self.usb_inventory
        ):
            raise CaseBlocked("no Gembird MSIS-PM (2) device is connected")

        return self.__manual_verdict(
            "CLI-HW-20",
            "Plug in the second Gembird MSIS-PM; run 'list'; toggle the single socket.",
            "Detected as Gembird MSIS-PM (2); 1 socket switches.",
        )

    def test_cli_hw_21(self) -> TestStatus:
        """
        @brief CLI-HW-21: Energenie EG-PMS plug-in; toggle each of 4 sockets. Physical case.
        """
        if not any(
            device["type"] == "Energenie EG-PMS" for device in self.usb_inventory
        ):
            raise CaseBlocked("no Energenie EG-PMS device is connected")

        return self.__manual_verdict(
            "CLI-HW-21",
            "Plug in the Energenie EG-PMS; run 'list'; toggle each of the 4 sockets.",
            "Detected as Energenie EG-PMS; serial read from device; 4 sockets switch.",
        )

    def test_cli_hw_22(self) -> TestStatus:
        """
        @brief CLI-HW-22: Energenie EG-PMS2 plug-in; toggle each of 4 sockets. Physical case.
        """
        if not any(
            device["type"] == "Energenie EG-PMS2" for device in self.usb_inventory
        ):
            raise CaseBlocked("no Energenie EG-PMS2 device is connected")

        return self.__manual_verdict(
            "CLI-HW-22",
            "Plug in the Energenie EG-PMS2; run 'list'; toggle each of the 4 sockets.",
            "Detected as Energenie EG-PMS2; 4 sockets switch.",
        )

    def test_cli_hw_23(self) -> TestStatus:
        """
        @brief CLI-HW-23: EG-PMxx-LAN discovery on the LAN, addressing by serial, toggling and
        status read-back. Physical case.
        """
        if not self.lan_inventory:
            raise CaseBlocked("no Energenie EG-PMxx-LAN device is connected")

        return self.__manual_verdict(
            "CLI-HW-23",
            "Discover the EG-PMxx-LAN device on the LAN; address it by serial; toggle each of "
            "the 4 sockets; read the status back.",
            "Detected via MAC/port; addressable by serial; 4 sockets switch; status echoed back.",
        )

    def test_cli_hw_24(self) -> TestStatus:
        """
        @brief CLI-HW-24: unplug a USB device mid-session. Physical case.
        """
        self.__require_first_usb()

        return self.__manual_verdict(
            "CLI-HW-24",
            "With a USB device connected, unplug it mid-session and run 'list' plus any "
            "power command.",
            "Device shown as disconnected; operations fail gracefully; no crash.",
        )

    def test_cli_hw_25(self) -> TestStatus:
        """
        @brief CLI-HW-25: unplug a LAN device mid-session. Physical case.
        """
        self.__require_first_lan()

        return self.__manual_verdict(
            "CLI-HW-25",
            "With an Ethernet device connected, unplug it mid-session and run "
            "'list --include-device-types ethernet' plus any power command.",
            "Device shown as disconnected; operations fail gracefully; no crash.",
        )

    def test_cli_hw_26(self) -> TestStatus:
        """
        @brief CLI-HW-26: Linux, any USB device: run without udev rules, then with them.
        Not applicable on Windows/macOS.
        """
        if self.system != System.LINUX:
            raise CaseBlocked("udev rules case does not apply on this platform")

        if not self.usb_inventory:
            raise CaseBlocked("no USB device is connected")

        return self.__manual_verdict(
            "CLI-HW-26",
            "On Linux, run without the udev rules installed, then install them and run again.",
            "Without rules: access denied / enumeration fails; with rules: works.",
        )

    def run_hardware_phase(self) -> None:
        """
        Run the whole hardware phase.
        """
        self.logger.info("Starting the hardware phase.")

        self.__prompt(
            "PHASE 2: make sure the power-strip hardware is connected and switched ON."
        )

        self.__clean_device_database()

        self.__enumerate_devices()

        if not self.usb_inventory and not self.lan_inventory:
            self.logger.warning(
                "No hardware was detected; recording hardware cases as blocked."
            )

        for case_id, test_function in [
            ("CLI-HW-01", self.test_cli_hw_01),
            ("CLI-HW-02", self.test_cli_hw_02),
            ("CLI-HW-03", self.test_cli_hw_03),
            ("CLI-HW-04", self.test_cli_hw_04),
            ("CLI-HW-05", self.test_cli_hw_05),
            ("CLI-HW-06", self.test_cli_hw_06),
            ("CLI-HW-07", self.test_cli_hw_07),
            ("CLI-HW-08", self.test_cli_hw_08),
            ("CLI-HW-09", self.test_cli_hw_09),
            ("CLI-HW-10", self.test_cli_hw_10),
            ("CLI-HW-11", self.test_cli_hw_11),
            ("CLI-HW-12", self.test_cli_hw_12),
            ("CLI-HW-13", self.test_cli_hw_13),
            ("CLI-HW-14", self.test_cli_hw_14),
            ("CLI-HW-15", self.test_cli_hw_15),
            ("CLI-HW-16", self.test_cli_hw_16),
            ("CLI-HW-17", self.test_cli_hw_17),
            ("CLI-HW-18", self.test_cli_hw_18),
            ("CLI-HW-19", self.test_cli_hw_19),
            ("CLI-HW-20", self.test_cli_hw_20),
            ("CLI-HW-21", self.test_cli_hw_21),
            ("CLI-HW-22", self.test_cli_hw_22),
            ("CLI-HW-23", self.test_cli_hw_23),
            ("CLI-HW-24", self.test_cli_hw_24),
            ("CLI-HW-25", self.test_cli_hw_25),
            ("CLI-HW-26", self.test_cli_hw_26),
        ]:
            self.__run_case(case_id, test_function)

        self.logger.info("Hardware phase completed.")

    # ---------------------------------------------------------------------------------------------
    #
    # Reporting.
    #
    # ---------------------------------------------------------------------------------------------
    def __print_results_table(self) -> None:
        """
        Print the final two-column results table: test id and status.
        """
        id_width = max([len(case_id) for case_id, _ in self.results] + [len("Test ID")])

        lines: list[str] = []
        header = f"{'Test ID'.ljust(id_width)}  Status"
        lines.append("=" * len(header))
        lines.append("CLI TEST RESULTS")
        lines.append("=" * len(header))
        lines.append(header)
        lines.append(f"{'-' * id_width}  ------")

        for case_id, status in self.results:
            lines.append(f"{case_id.ljust(id_width)}  {status.value}")

        summary = ", ".join(
            f"{status.value}: {sum(1 for _, recorded in self.results if recorded == status)}"
            for status in TestStatus
        )
        lines.append("-" * len(header))
        lines.append(summary)

        report = "\n".join(lines)
        print("\n" + report)
        self.logger.info("\n%s", report)

    def run(self) -> None:
        """
        Run both test phases and print the results table.
        """
        self.logger.info("Starting the CLI test run.")

        self.run_no_hardware_phase()
        self.run_hardware_phase()

        self.__print_results_table()

        failures = sum(1 for _, status in self.results if status == TestStatus.FAIL)
        self.logger.info("CLI test run completed. Failed cases: %d", failures)

        if failures:
            sys.exit(1)


# --------------------------------------------------------------------------------------------------
#
# Entry point.
#
# --------------------------------------------------------------------------------------------------
if __name__ == "__main__":
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s [%(levelname)s] %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
        handlers=[
            logging.StreamHandler(sys.stdout),
            logging.FileHandler("cli_tests.log", mode="w"),
        ],
    )

    entry_logger = logging.getLogger("cli-tests")

    parser = argparse.ArgumentParser(
        description="Runs the sokketter-cli test cases from TESTING.md against a built binary."
    )

    parser.add_argument(
        "--folder",
        type=str,
        default=None,
        metavar="FOLDER",
        help="Path to the folder containing the sokketter-cli binary.",
    )

    parser.add_argument(
        "--storage-path",
        type=str,
        default=None,
        metavar="STORAGE_PATH",
        help="Override of the sokketter storage path (default: platform-specific location).",
    )

    parser.add_argument(
        "--assume-hardware",
        action="store_true",
        help="Skip the interactive prompts asking to disconnect/connect the hardware.",
    )

    args = parser.parse_args()

    folder = args.folder

    if not folder:
        try:
            folder = input("Path to the folder containing sokketter-cli: ").strip()
        except EOFError:
            folder = ""

    if not folder:
        entry_logger.error(
            "No folder was provided. Provide --folder or enter the path interactively."
        )
        sys.exit(1)

    try:
        CliTests(folder, args.storage_path, args.assume_hardware).run()
    except FileNotFoundError as error:
        entry_logger.error("%s", error)
        sys.exit(1)
