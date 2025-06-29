#!/usr/bin/env python

# --------------------------------------------------------------------------------------------------
#
# Imports.
#
# --------------------------------------------------------------------------------------------------
import argparse
import os
import sys
from typing import List

# --------------------------------------------------------------------------------------------------
#
# Global variables.
#
# --------------------------------------------------------------------------------------------------


# --------------------------------------------------------------------------------------------------
#
# Class definition.
#
# --------------------------------------------------------------------------------------------------
class ProjectVersion:
    """
    Class to manage the project version.
    """

    def __init__(self):
        """
        Initialize the ProjectVersion class.
        """
        self.__current_working_directory = os.getcwd()
        self.__version_filepath = os.path.join(
            self.__current_working_directory, "ProjectVersion.cmake"
        )

    def increment(self, version_part: str):
        """
        Increment the specified version part.
        """
        if version_part == "NONE":
            sys.exit(0)

        version_file_lines: List[str] = []
        with open(
            file=self.__version_filepath, mode="r", encoding="utf-8"
        ) as version_file:
            version_file_lines = version_file.readlines()

        for line_index, line in enumerate(version_file_lines):
            if version_part == "MAJOR":
                if line.startswith("set(SOKKETTER_VERSION_MAJOR"):
                    version_part_int = int(
                        line.replace("set(SOKKETTER_VERSION_MAJOR", "")
                        .replace(")", "")
                        .strip()
                    )
                    incremented_version_part = version_part_int + 1
                    version_file_lines[line_index] = (
                        f"set(SOKKETTER_VERSION_MAJOR {incremented_version_part})\n"
                    )

                if line.startswith("set(SOKKETTER_VERSION_MINOR"):
                    version_file_lines[line_index] = "set(SOKKETTER_VERSION_MINOR 0)\n"

                if line.startswith("set(SOKKETTER_VERSION_MICRO"):
                    version_file_lines[line_index] = "set(SOKKETTER_VERSION_MICRO 0)\n"

                if line.startswith("set(SOKKETTER_VERSION_NANO"):
                    version_file_lines[line_index] = "set(SOKKETTER_VERSION_NANO 0)\n"

            elif version_part == "MINOR":
                if line.startswith("set(SOKKETTER_VERSION_MINOR"):
                    version_part_int = int(
                        line.replace("set(SOKKETTER_VERSION_MINOR", "")
                        .replace(")", "")
                        .strip()
                    )
                    incremented_version_part = version_part_int + 1
                    version_file_lines[line_index] = (
                        f"set(SOKKETTER_VERSION_MINOR {incremented_version_part})\n"
                    )

                if line.startswith("set(SOKKETTER_VERSION_MICRO"):
                    version_file_lines[line_index] = "set(SOKKETTER_VERSION_MICRO 0)\n"

                if line.startswith("set(SOKKETTER_VERSION_NANO"):
                    version_file_lines[line_index] = "set(SOKKETTER_VERSION_NANO 0)\n"

            elif version_part == "MICRO":
                if line.startswith("set(SOKKETTER_VERSION_MICRO"):
                    version_part_int = int(
                        line.replace("set(SOKKETTER_VERSION_MICRO", "")
                        .replace(")", "")
                        .strip()
                    )
                    incremented_version_part = version_part_int + 1
                    version_file_lines[line_index] = (
                        f"set(SOKKETTER_VERSION_MICRO {incremented_version_part})\n"
                    )

                if line.startswith("set(SOKKETTER_VERSION_NANO"):
                    version_file_lines[line_index] = "set(SOKKETTER_VERSION_NANO 0)\n"

            elif version_part == "NANO":
                if line.startswith("set(SOKKETTER_VERSION_NANO"):
                    version_part_int = int(
                        line.replace("set(SOKKETTER_VERSION_NANO", "")
                        .replace(")", "")
                        .strip()
                    )
                    incremented_version_part = version_part_int + 1
                    version_file_lines[line_index] = (
                        f"set(SOKKETTER_VERSION_NANO {incremented_version_part})\n"
                    )

        with open(
            file=self.__version_filepath, mode="w", encoding="utf-8"
        ) as version_file:
            version_file.writelines(version_file_lines)

    def get(self) -> str:
        """
        Get the current project version.
        """
        version_file_lines: List[str] = []
        with open(
            file=self.__version_filepath, mode="r", encoding="utf-8"
        ) as version_file:
            version_file_lines = version_file.readlines()

        version_major = 0
        version_minor = 0
        version_micro = 0
        version_nano = 0

        for _, line in enumerate(version_file_lines):
            if line.startswith("set(SOKKETTER_VERSION_MAJOR"):
                version_major = int(
                    line.replace("set(SOKKETTER_VERSION_MAJOR", "")
                    .replace(")", "")
                    .strip()
                )

            elif line.startswith("set(SOKKETTER_VERSION_MINOR"):
                version_minor = int(
                    line.replace("set(SOKKETTER_VERSION_MINOR", "")
                    .replace(")", "")
                    .strip()
                )

            elif line.startswith("set(SOKKETTER_VERSION_MICRO"):
                version_micro = int(
                    line.replace("set(SOKKETTER_VERSION_MICRO", "")
                    .replace(")", "")
                    .strip()
                )

            elif line.startswith("set(SOKKETTER_VERSION_NANO"):
                version_nano = int(
                    line.replace("set(SOKKETTER_VERSION_NANO", "")
                    .replace(")", "")
                    .strip()
                )

        if version_nano != 0:
            return f"{version_major}.{version_minor}.{version_micro}.{version_nano}"

        return f"{version_major}.{version_minor}.{version_micro}"


# --------------------------------------------------------------------------------------------------
#
# Entry point.
#
# --------------------------------------------------------------------------------------------------
if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument(
        dest="action",
        type=str,
        nargs="?",
        choices=[
            "NONE",
            "INCREMENT_VERSION",
            "GET_VERSION",
        ],
        default="NONE",
        const="NONE",
    )

    parser.add_argument(
        dest="version_part",
        type=str,
        nargs="?",
        choices=[
            "NONE",
            "MAJOR",
            "MINOR",
            "MICRO",
            "NANO",
        ],
        default="NONE",
        const="NONE",
    )

    args = parser.parse_args()

    if args.action == "NONE":
        sys.exit(0)

    elif args.action == "INCREMENT_VERSION":
        ProjectVersion().increment(args.version_part)

    elif args.action == "GET_VERSION":
        print(ProjectVersion().get())
