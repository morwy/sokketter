#!/usr/bin/env python

# --------------------------------------------------------------------------------------------------
#
# Imports.
#
# --------------------------------------------------------------------------------------------------
import argparse
import platform
import re
import sys
from enum import Enum


# --------------------------------------------------------------------------------------------------
#
# Global variables.
#
# --------------------------------------------------------------------------------------------------
class Action(str, Enum):
    """
    Enum to define the actions that can be performed by the Environment class.
    """

    NONE = "NONE"
    GET_OS_NAME = "GET_OS_NAME"
    GET_OS_VERSION = "GET_OS_VERSION"
    GET_ARCHITECTURE = "GET_ARCHITECTURE"


# --------------------------------------------------------------------------------------------------
#
# Class definition.
#
# --------------------------------------------------------------------------------------------------
class Environment:
    """
    Class to manage the environment variables.
    """

    @staticmethod
    def get_os_name() -> str:
        """
        Get the name of the operating system.
        """
        os_name = platform.system().lower()

        if os_name == "windows":
            return "windows"

        if os_name == "linux":
            try:
                with open(file="/etc/os-release", mode="r", encoding="utf-8") as file:
                    os_release = file.read()

                match = re.search(r'^ID="?([^"\n]+)"?', os_release, re.MULTILINE)
                if not match:
                    raise EnvironmentError("Unable to determine Linux distribution ID")

                return match.group(1).lower()

            except FileNotFoundError:
                return "linux"

        if os_name == "darwin":
            return "macos"

        raise EnvironmentError("Unsupported operating system")

    @staticmethod
    def get_os_version() -> str:
        """
        Get the version of the operating system.
        """
        os_name = platform.system().lower()

        if os_name == "windows":
            return platform.release()

        if os_name == "linux":
            try:
                with open(file="/etc/os-release", mode="r", encoding="utf-8") as file:
                    os_release = file.read()

                version_match = re.search(
                    r'^VERSION_ID="?([^"\n]+)"?', os_release, re.MULTILINE
                )
                if not version_match:
                    raise EnvironmentError("Unable to determine Linux version")

                return version_match.group(1)

            except FileNotFoundError:
                return platform.release()

        if os_name == "darwin":
            mac_version = platform.mac_ver()[0]
            if not mac_version:
                raise EnvironmentError("Unable to determine macOS version")

            mac_version_parts = mac_version.split(".")
            version = f"{mac_version_parts[0]}.{mac_version_parts[1]}"

            return version

        raise EnvironmentError("Unsupported OS version")

    @staticmethod
    def get_architecture() -> str:
        """
        Get the architecture of the machine.
        """
        machine = platform.machine().lower()

        if machine in ["arm64", "aarch64"]:
            return "arm64"

        if machine in ["x86_64", "amd64"]:
            return "x86_64"

        raise EnvironmentError("Unsupported architecture")


# --------------------------------------------------------------------------------------------------
#
# Entry point.
#
# --------------------------------------------------------------------------------------------------
if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "-a",
        "--action",
        type=str,
        nargs="?",
        metavar="ACTION",
        choices=[action.value for action in Action],
        default=Action.NONE.value,
        const=Action.NONE.value,
        help=f"Action to perform (default: {Action.NONE.name}). Available actions: {', '.join(action.value for action in Action)}.",
    )

    args = parser.parse_args()

    if args.action == Action.NONE.value:
        print("No action specified. Use --help to see available actions.")
        sys.exit(1)

    elif args.action == Action.GET_OS_NAME.value:
        print(Environment.get_os_name())

    elif args.action == Action.GET_OS_VERSION.value:
        print(Environment.get_os_version())

    elif args.action == Action.GET_ARCHITECTURE.value:
        print(Environment.get_architecture())
