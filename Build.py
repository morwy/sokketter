#!/usr/bin/env python

"""
Build script for the project.

Project is intended to be built using C++17, Qt6 and tools bundled with Qt6 (CMake, Ninja, etc.).
It expects the Qt6 installation to be done either via the official Qt installer or via aqt package manager.

This script is designed to be cross-platform and should work on Windows, Linux, and macOS.

It handles the build process, including configuration, compilation, testing, and packaging of
the application. The script is designed to be run from the command line and can be integrated
into CI/CD pipelines.
"""

# --------------------------------------------------------------------------------------------------
#
# Imports.
#
# --------------------------------------------------------------------------------------------------
import argparse
import glob
import logging
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile
import time
from enum import Enum

from Environment import Architecture, Environment, System
from ProjectVersion import ProjectVersion


# --------------------------------------------------------------------------------------------------
#
# Global variables.
#
# --------------------------------------------------------------------------------------------------
class BuildStage(str, Enum):
    """
    Enum to define the build stages.
    """

    ALL = "ALL"
    CLEAN = "CLEAN"
    CONFIGURE = "CONFIGURE"
    BUILD = "BUILD"
    VERIFY = "VERIFY"
    PACKAGE = "PACKAGE"
    TEST = "TEST"


# --------------------------------------------------------------------------------------------------
#
# Class definition.
#
# --------------------------------------------------------------------------------------------------
class Build:
    """
    Class to handle the build process.
    """

    def __init__(
        self, stages: list[str], qt_version: str, architecture: Architecture
    ) -> None:
        """
        Initialize the build class.
        """
        self.logger = logging.getLogger(__name__)
        self.logger.info("Build class initialized.")

        self.stages = stages
        self.logger.info("Specified stages: %s", self.stages)

        self.system = Environment.get_os()
        self.logger.info("Operating system: %s", self.system.value)

        self.os_version = Environment.get_os_version()
        self.logger.info("Operating system version: %s", self.os_version)

        self.architecture = architecture
        self.logger.info("Target architecture: %s", self.architecture.value)

        self.qt_version = qt_version
        self.logger.info("Target Qt version: %s", self.qt_version)

        (
            self.qt_version,
            self.qt_root_folder,
            self.qt_version_folder,
            self.qt_cmake_folder,
        ) = self.__resolve_qt6_package()
        self.logger.info("Selected Qt version: %s", self.qt_version)
        self.logger.info("Selected Qt root folder: %s", self.qt_root_folder)
        self.logger.info("Selected Qt version folder: %s", self.qt_version_folder)
        self.logger.info("Selected Qt CMake folder: %s", self.qt_cmake_folder)

        self.cmake = self.__get_cmake()
        self.logger.info("Target CMake executable: %s", self.cmake)

        self.compiler = self.__get_cpp_compiler()
        self.logger.info("Target C++ compiler: %s", self.compiler)

        self.windows_msvc_env_script: str | None = None
        if self.system == System.WINDOWS:
            self.windows_msvc_env_script = self.__resolve_windows_msvc_env_script()
            if self.windows_msvc_env_script:
                self.logger.info(
                    "Visual Studio developer environment script: %s",
                    self.windows_msvc_env_script,
                )
            else:
                raise RuntimeError("Visual Studio environment script is not available.")

        self.version = ProjectVersion().get()
        self.logger.info("Project version: %s", self.version)

        self.workspace = os.environ.get(
            "GITHUB_WORKSPACE", os.path.dirname(os.path.abspath(__file__))
        )
        self.logger.info("Workspace: %s", self.workspace)

        self.temp_build_output_dir = os.path.join(self.workspace, "build")
        self.logger.info("Temp build output directory: %s", self.temp_build_output_dir)

        self.temp_binary_output_dir = self.__construct_binary_output_dir(self.workspace)
        self.logger.info(
            "Temp binary output directory: %s", self.temp_binary_output_dir
        )

        self.results_output_dir = os.path.join(self.workspace, "results")
        self.logger.info("Results output directory: %s", self.results_output_dir)
        os.makedirs(self.results_output_dir, exist_ok=True)

    def __get_cmake(self) -> str:
        """
        Get the CMake executable from the environment variable.
        """
        executable_name = "cmake.exe" if self.system == System.WINDOWS else "cmake"

        qt_tools_dir = pathlib.Path(self.qt_root_folder) / "Tools"
        qt_cmake_glob_pattern = os.path.join("CMake*", "**", "bin", executable_name)
        qt_cmake_candidates = list(qt_tools_dir.rglob(qt_cmake_glob_pattern))

        for candidate in qt_cmake_candidates:
            self.logger.info("Found CMake candidate: %s", candidate.resolve())
            if candidate.exists():
                return str(candidate)

        raise EnvironmentError("CMake executable not found")

    def __get_cpp_compiler(self) -> str:
        """
        Get the C++ compiler from the environment variable.
        """
        compiler = ""

        if self.system == System.WINDOWS:
            compiler = "cl"
        elif self.system == System.MACOS:
            compiler = "clang++"
        elif self.system == System.LINUX:
            compiler = "g++"
        else:
            self.logger.error("Unsupported platform: %s", self.system)
            raise EnvironmentError("Unsupported platform")

        return compiler

    def __construct_binary_output_dir(self, workspace: str) -> str:
        """
        Get the binary output directory based on the platform.
        """
        return os.path.join(
            workspace,
            "bin",
            f"{self.system.value}_{self.architecture.value}",
            "Release",
        )

    def __get_debian_architecture(self) -> str:
        """
        Get the Debian package architecture name for the current platform.
        """
        if self.architecture == Architecture.X86_64:
            return "amd64"

        if self.architecture == Architecture.ARM64:
            return "arm64"

        raise EnvironmentError(
            f"Unsupported Debian package architecture: {self.architecture.value}"
        )

    def __get_linuxdeployqt_architecture(self) -> str:
        """
        Get the linuxdeployqt continuous release architecture token for the current platform.
        """
        if self.architecture == Architecture.X86_64:
            return "x86_64"

        if self.architecture == Architecture.ARM64:
            return "aarch64"

        raise EnvironmentError(
            f"Unsupported linuxdeployqt architecture: {self.architecture.value}"
        )

    def __resolve_qt6_package(self) -> tuple[str, str, str, str]:
        """
        Find the Qt6 package matching the requested version and target architecture.
        """

        version_pattern = re.compile(r"^(\d+)\.(\d+)(?:\.(\d+))?$")

        def has_qt6_config(path: pathlib.Path) -> bool:
            return (path / "Qt6Config.cmake").exists() or (
                path / "qt6-config.cmake"
            ).exists()

        def get_qt_version(path: pathlib.Path) -> tuple[int, int, int] | None:
            for parent in [path, *path.parents]:
                match = version_pattern.fullmatch(parent.name)
                if match:
                    return (
                        int(match.group(1)),
                        int(match.group(2)),
                        int(match.group(3) or 0),
                    )

            config_version = path / "Qt6ConfigVersion.cmake"
            if not config_version.exists():
                config_version = path / "qt6-config-version.cmake"

            if config_version.exists():
                contents = config_version.read_text(encoding="utf-8", errors="ignore")
                match = re.search(
                    r"PACKAGE_VERSION\s+\"(\d+)\.(\d+)(?:\.(\d+))?\"", contents
                )
                if match:
                    return (
                        int(match.group(1)),
                        int(match.group(2)),
                        int(match.group(3) or 0),
                    )

            return None

        def qt6_dirs_from_prefix(prefix: pathlib.Path) -> list[pathlib.Path]:
            return [prefix, prefix / "lib" / "cmake" / "Qt6"]

        def is_qt_dir_arch_compatible(path: pathlib.Path) -> bool:
            if self.system != System.WINDOWS:
                return True

            normalized = str(path).lower()

            if self.architecture == Architecture.X86_64:
                return "arm64" not in normalized

            if self.architecture == Architecture.ARM64:
                return "arm64" in normalized

            return True

        user_profile = os.environ.get("USERPROFILE", "")

        candidates: list[pathlib.Path] = []
        qt6_dir = os.environ.get("Qt6_DIR") or os.environ.get("QT6_DIR")
        if qt6_dir:
            candidates.append(pathlib.Path(qt6_dir).expanduser().resolve())

        qt_root_dir = os.environ.get("QT_ROOT_DIR")
        if qt_root_dir:
            candidates.extend(
                qt6_dirs_from_prefix(pathlib.Path(qt_root_dir).expanduser().resolve())
            )

        cmake_prefix_path = os.environ.get("CMAKE_PREFIX_PATH", "")
        for prefix in cmake_prefix_path.split(os.pathsep):
            if prefix:
                candidates.extend(
                    qt6_dirs_from_prefix(pathlib.Path(prefix).expanduser().resolve())
                )

        home_dir = os.environ.get("HOME", "")

        if self.system == System.MACOS:
            patterns = [
                os.path.join(home_dir, "Qt", "*", "macos", "lib", "cmake", "Qt6"),
            ]
        elif self.system == System.WINDOWS:
            patterns = [
                os.path.join("C:\\Qt", "*", "msvc*", "lib", "cmake", "Qt6"),
                os.path.join("C:\\Qt", "*", "mingw*", "lib", "cmake", "Qt6"),
                os.path.join(user_profile, "Qt", "*", "msvc*", "lib", "cmake", "Qt6"),
                os.path.join(user_profile, "Qt", "*", "mingw*", "lib", "cmake", "Qt6"),
            ]
        elif self.system == System.LINUX:
            patterns = [
                os.path.join(home_dir, "Qt", "*", "gcc_64", "lib", "cmake", "Qt6"),
                os.path.join(
                    home_dir, "Qt", "*", "linux_gcc_64", "lib", "cmake", "Qt6"
                ),
                os.path.join("/opt", "Qt", "*", "gcc_64", "lib", "cmake", "Qt6"),
                os.path.join("/usr", "lib", "*", "cmake", "Qt6"),
                os.path.join("/usr", "lib", "cmake", "Qt6"),
                os.path.join("/usr", "local", "lib", "cmake", "Qt6"),
            ]
        else:
            patterns = []

        for pattern in patterns:
            for match in sorted(glob.glob(pattern), reverse=True):
                candidates.append(pathlib.Path(match).resolve())

        packages: list[tuple[tuple[int, int, int], pathlib.Path]] = []
        seen: set[pathlib.Path] = set()
        for candidate in candidates:
            if candidate in seen or not has_qt6_config(candidate):
                continue
            seen.add(candidate)
            if not is_qt_dir_arch_compatible(candidate):
                continue
            version = get_qt_version(candidate)
            if version is not None:
                packages.append((version, candidate))

        requested_match = version_pattern.fullmatch(self.qt_version)
        if self.qt_version == "latest":
            matching_packages = packages
        elif requested_match:
            requested_version = tuple(
                int(component or 0) for component in requested_match.groups()
            )
            if requested_match.group(3) is None:
                matching_packages = [
                    (version, path)
                    for version, path in packages
                    if version[:2] == requested_version[:2]
                ]
            else:
                matching_packages = [
                    (version, path)
                    for version, path in packages
                    if version == requested_version
                ]
        else:
            matching_packages = []

        if matching_packages:
            selected_version, selected_folder = max(
                matching_packages, key=lambda package: package[0]
            )
            return (
                ".".join(str(component) for component in selected_version),
                str(selected_folder.parent.parent.parent.parent.parent),
                str(selected_folder.parent.parent.parent.parent),
                str(selected_folder),
            )

        raise EnvironmentError(
            f"No suitable Qt6 package found for version '{self.qt_version}' and "
            f"architecture '{self.architecture.value}'."
        )

    def __resolve_qt_tool(self, tool_name: str) -> str:
        """
        Resolve Qt deployment tools (e.g. macdeployqt, windeployqt) to an executable path.
        """
        executable_name = tool_name
        if self.system == System.WINDOWS and not tool_name.endswith(".exe"):
            executable_name = f"{tool_name}.exe"

        qt_kit_folder = pathlib.Path(self.qt_cmake_folder).parent.parent.parent
        candidate = qt_kit_folder / "bin" / executable_name
        if candidate.exists():
            return str(candidate)

        raise FileNotFoundError(
            f"Could not find '{tool_name}'. Add Qt's bin directory to PATH or set Qt6_DIR/QT6_DIR."
        )

    def __msvc_arch_token(self) -> str:
        """
        Map the detected architecture to the token MSVC dev environment scripts expect.
        """
        if self.architecture == Architecture.ARM64:
            return "arm64"

        return "x64"

    def __resolve_windows_msvc_env_script(self) -> str | None:
        """
        Resolve a Visual Studio developer environment batch script path.
        """
        arch_token = self.__msvc_arch_token()

        vswhere = os.path.join(
            os.environ.get("ProgramFiles(x86)", "C:\\Program Files (x86)"),
            "Microsoft Visual Studio",
            "Installer",
            "vswhere.exe",
        )

        if os.path.exists(vswhere):
            script_patterns = [
                "Common7\\Tools\\VsDevCmd.bat",
                f"VC\\Auxiliary\\Build\\vcvars{arch_token}.bat",
                "VC\\Auxiliary\\Build\\vcvarsall.bat",
            ]

            required_component = (
                "Microsoft.VisualStudio.Component.VC.Tools.ARM64"
                if arch_token == "arm64"
                else "Microsoft.VisualStudio.Component.VC.Tools.x86.x64"
            )

            for pattern in script_patterns:
                result = subprocess.run(
                    [
                        vswhere,
                        "-latest",
                        "-products",
                        "*",
                        "-requires",
                        required_component,
                        "-find",
                        pattern,
                    ],
                    check=False,
                    capture_output=True,
                    text=True,
                )

                candidate = result.stdout.strip()
                if candidate and os.path.exists(candidate):
                    return candidate

        fallback_roots = [
            os.path.join(
                "C:\\Program Files",
                "Microsoft Visual Studio",
                "2022",
                "Community",
            ),
            os.path.join(
                "C:\\Program Files",
                "Microsoft Visual Studio",
                "2022",
                "BuildTools",
            ),
        ]

        fallback_paths = [
            os.path.join("Common7", "Tools", "VsDevCmd.bat"),
            os.path.join("VC", "Auxiliary", "Build", f"vcvars{arch_token}.bat"),
            os.path.join("VC", "Auxiliary", "Build", "vcvarsall.bat"),
        ]

        for root in fallback_roots:
            for rel_path in fallback_paths:
                candidate = os.path.join(root, rel_path)
                if os.path.exists(candidate):
                    return candidate

        return None

    def __get_windows_msvc_environment(self) -> dict[str, str]:
        """
        Capture the environment initialized by the Visual Studio developer command file.
        """
        if self.windows_msvc_env_script is None:
            raise RuntimeError("Visual Studio environment script is not available.")

        arch_token = self.__msvc_arch_token()

        script_name = os.path.basename(self.windows_msvc_env_script).lower()
        capture_environment = os.environ.copy()
        capture_environment["SOKKETTER_MSVC_ENV_SCRIPT"] = self.windows_msvc_env_script
        if script_name == "vsdevcmd.bat":
            vcvars_call = (
                'call "%SOKKETTER_MSVC_ENV_SCRIPT%" '
                f"-arch={arch_token} -host_arch={arch_token} >nul"
            )
        elif script_name == "vcvarsall.bat":
            vcvars_call = 'call "%SOKKETTER_MSVC_ENV_SCRIPT%" ' f"{arch_token} >nul"
        else:
            vcvars_call = 'call "%SOKKETTER_MSVC_ENV_SCRIPT%" >nul'

        # Keep the original command out of the batch file. list2cmdline() quotes for
        # CreateProcess, but does not escape metacharacters interpreted by cmd.exe.
        wrapper_content = (
            "@echo off\n"
            f"{vcvars_call}\n"
            "if errorlevel 1 exit /b %errorlevel%\n"
            "set\n"
        )

        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".cmd", delete=False, encoding="utf-8"
        ) as wrapper_file:
            wrapper_file.write(wrapper_content)
            wrapper_path = wrapper_file.name

        try:
            result = subprocess.run(
                [
                    "cmd.exe",
                    "/d",
                    "/c",
                    "call",
                    wrapper_path,
                ],
                check=True,
                capture_output=True,
                text=True,
                env={
                    **capture_environment,
                },
            )
        finally:
            if os.path.exists(wrapper_path):
                os.remove(wrapper_path)

        environment = os.environ.copy()
        for line in result.stdout.splitlines():
            name, separator, value = line.partition("=")
            if separator:
                environment[name] = value

        return environment

    def __execute_command(self, cmake_command, cwd: str | None = None):
        try:
            command_to_run = cmake_command
            command_environment = None

            if (
                self.system == System.WINDOWS
                and isinstance(command_to_run, list)
                and self.windows_msvc_env_script is not None
            ):
                command_environment = self.__get_windows_msvc_environment()

            self.logger.info(
                "Executing command: %s",
                (
                    " ".join(command_to_run)
                    if isinstance(command_to_run, list)
                    else command_to_run
                ),
            )

            with subprocess.Popen(
                command_to_run,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                cwd=cwd,
                env=command_environment,
                shell=False,
            ) as process:
                if process.stdout is not None:
                    for line in process.stdout:
                        self.logger.info(line.rstrip())

                process.wait()
                if process.returncode != 0:
                    raise subprocess.CalledProcessError(
                        process.returncode, command_to_run
                    )

        except FileNotFoundError as e:
            self.logger.error("Command executable not found: %s", e)
            raise

        except subprocess.CalledProcessError as e:
            self.logger.error("Command failed with error:\n%s", e.stderr)
            raise

    def __get_cached_cmake_generator(self, build_dir: str) -> str | None:
        """
        Read the CMake generator recorded in CMakeCache.txt, if available.
        """
        cache_file = os.path.join(build_dir, "CMakeCache.txt")
        if not os.path.exists(cache_file):
            return None

        with open(cache_file, mode="r", encoding="utf-8", errors="ignore") as file:
            for line in file:
                if line.startswith("CMAKE_GENERATOR:INTERNAL="):
                    return line.strip().split("=", maxsplit=1)[1]

        return None

    def __reset_cmake_cache(self, build_dir: str) -> None:
        """
        Remove CMake cache artifacts to allow reconfiguring with a different generator.
        """
        cache_file = os.path.join(build_dir, "CMakeCache.txt")
        cache_dir = os.path.join(build_dir, "CMakeFiles")

        if os.path.exists(cache_file):
            os.remove(cache_file)

        if os.path.exists(cache_dir):
            shutil.rmtree(cache_dir)

    def __get_cmake_generator(self):
        ninja_executable = "ninja.exe" if self.system == System.WINDOWS else "ninja"
        ninja_filepath = os.path.join(
            self.qt_root_folder, "Tools", "Ninja", ninja_executable
        )
        desired_generator = "Ninja"

        if os.path.exists(ninja_filepath):
            self.logger.info(
                "Using Ninja generator because Ninja is available at: %s",
                ninja_filepath,
            )
            os.environ["PATH"] = os.pathsep.join(
                [os.path.dirname(ninja_filepath), os.environ.get("PATH", "")]
            )
            desired_generator = "Ninja"
        else:
            self.logger.info(
                "Ninja not found at: %s!",
                ninja_filepath,
            )
            raise EnvironmentError(
                "Ninja build system is required but not found. Please ensure Ninja is installed and available in the PATH."
            )

        cached_generator = self.__get_cached_cmake_generator(self.temp_build_output_dir)
        if cached_generator and cached_generator != desired_generator:
            self.logger.info(
                "Switching CMake generator from '%s' to '%s'. Clearing stale cache.",
                cached_generator,
                desired_generator,
            )

            self.__reset_cmake_cache(self.temp_build_output_dir)

            deps_dir = os.path.join(self.temp_build_output_dir, "_deps")
            if os.path.exists(deps_dir):
                shutil.rmtree(deps_dir)

        return desired_generator

    def __remove_directory(self, directory: str) -> None:
        """
        Remove a directory, retrying transient Windows races with indexers.
        """
        retry_count = 5
        retry_delay_seconds = 0.5

        for attempt in range(retry_count):
            try:
                shutil.rmtree(directory)
                return
            except OSError:
                if attempt == retry_count - 1:
                    raise

                self.logger.warning(
                    "Directory removal was interrupted; retrying: %s",
                    directory,
                )
                time.sleep(retry_delay_seconds)

    def __configure_finder(self, mount_point: pathlib.Path):
        script = f"""
        tell application "Finder"
            tell disk "{mount_point.name}"
                open

                set current view of container window to icon view
                set toolbar visible of container window to false
                set statusbar visible of container window to false

                tell icon view options of container window
                    set icon size to 64
                    set arrangement to not arranged
                    set text size to 12
                    set shows item info to false
                    set shows icon preview to true
                end tell

                set background picture of icon view options of container window to file ".background:background.tiff"

                -- 660 x 400 window
                set bounds of container window to {{100, 100, 760, 532}}

                -- Centered horizontally, roughly around the middle of the window
                set position of item "sokketter-ui.app" to {{185, 195}}
                set position of item "Applications" to {{475, 195}}

                close
                open
                update without registering applications
                delay 2
                close
            end tell
        end tell
        """

        subprocess.run(
            ["osascript", "-e", script],
            check=True,
        )

    def __detach_volume(self, mount_point: pathlib.Path) -> None:
        """
        Detach a mounted DMG volume, retrying and forcing detach if it is still busy
        (e.g. Finder hasn't released its handle on the volume yet). A prior attempt
        may still succeed asynchronously even after reporting "Resource busy", so the
        mount point is checked before each further attempt to avoid spurious
        "No such file or directory" failures on an already-detached volume.
        """
        if not mount_point.exists():
            return

        try:
            self.__run_hdiutil_with_retry(
                ["hdiutil", "detach", str(mount_point)],
                max_attempts=4,
                capture_output=True,
                text=True,
                stop_if_missing=mount_point,
            )
        except subprocess.CalledProcessError:
            if not mount_point.exists():
                return

            self.__run_hdiutil_with_retry(
                ["hdiutil", "detach", "-force", str(mount_point)],
                max_attempts=1,
                capture_output=True,
                text=True,
            )

    def __run_hdiutil_with_retry(
        self,
        command: list[str],
        max_attempts: int = 5,
        stop_if_missing: pathlib.Path | None = None,
        **kwargs,
    ) -> subprocess.CompletedProcess:
        """
        Run an hdiutil command, retrying on transient "Resource busy" failures
        (disk arbitration can briefly hold a lock on shared CI runners).
        """
        for attempt in range(1, max_attempts + 1):
            result = subprocess.run(command, check=False, **kwargs)
            if result.returncode == 0:
                return result

            if stop_if_missing is not None and not stop_if_missing.exists():
                return result

            stderr = getattr(result, "stderr", None) or ""
            self.logger.warning(
                "hdiutil command failed (attempt %d/%d): %s",
                attempt,
                max_attempts,
                stderr.strip() if isinstance(stderr, str) else stderr,
            )

            if attempt == max_attempts:
                raise subprocess.CalledProcessError(result.returncode, command)

            time.sleep(min(3 * attempt, 30))

        raise RuntimeError("Unreachable")

    def __create_dmg(self, app_path: str, output_path: str):
        app = pathlib.Path(app_path).resolve()
        output = pathlib.Path(output_path).resolve()

        # Detach any leftover volume from a previous failed run before starting.
        stale_mount_point = pathlib.Path("/Volumes/sokketter-ui")
        if stale_mount_point.exists():
            self.__detach_volume(stale_mount_point)

        with tempfile.TemporaryDirectory() as tmp:
            staging = pathlib.Path(tmp) / "sokketter-ui"
            staging.mkdir()

            # Prevent Spotlight from indexing the mounted volume; mdworker locking
            # a just-unmounted image is a common cause of hdiutil convert failing
            # with "Resource temporarily unavailable" on macOS CI runners.
            (staging / ".metadata_never_index").touch()

            # Copy application
            shutil.copytree(app, staging / app.name)

            # Applications symlink
            (staging / "Applications").symlink_to("/Applications")

            # Hidden folder holding the Finder background image
            background_dir = staging / ".background"
            background_dir.mkdir()
            shutil.copy(
                os.path.join(self.workspace, "sokketter-ui", "background.tiff"),
                background_dir / "background.tiff",
            )

            # Create read/write DMG first
            rw_dmg = pathlib.Path(tmp) / "sokketter-rw.dmg"

            self.__run_hdiutil_with_retry(
                [
                    "hdiutil",
                    "create",
                    "-volname",
                    "sokketter-ui",
                    "-srcfolder",
                    str(staging),
                    "-ov",
                    "-format",
                    "UDRW",
                    str(rw_dmg),
                ],
                capture_output=True,
                text=True,
            )

            # Mount it
            result = self.__run_hdiutil_with_retry(
                [
                    "hdiutil",
                    "attach",
                    "-readwrite",
                    "-noverify",
                    "-noautoopen",
                    str(rw_dmg),
                ],
                capture_output=True,
                text=True,
            )

            # Find mounted volume
            mount_point = None
            for line in result.stdout.splitlines():
                if "/Volumes/sokketter-ui" in line:
                    mount_point = pathlib.Path(line.split("\t")[-1])
                    break

            if not mount_point:
                raise RuntimeError("Could not find mounted DMG")

            # Belt-and-braces: also disable indexing directly on the mounted volume.
            subprocess.run(["mdutil", "-i", "off", str(mount_point)], check=False)

            try:
                self.__configure_finder(mount_point)

                # Give Finder a moment to fully release the volume before detaching.
                time.sleep(2)

                # Unmount
                self.__detach_volume(mount_point)

                # Compress to final DMG
                self.__run_hdiutil_with_retry(
                    [
                        "hdiutil",
                        "convert",
                        str(rw_dmg),
                        "-format",
                        "UDZO",
                        "-imagekey",
                        "zlib-level=9",
                        "-ov",
                        "-o",
                        str(output),
                    ],
                    max_attempts=10,
                    capture_output=True,
                    text=True,
                )

            finally:
                # Best effort cleanup if something failed; never raise from here.
                try:
                    self.__run_hdiutil_with_retry(
                        ["hdiutil", "detach", "-force", str(mount_point)],
                        max_attempts=1,
                        stdout=subprocess.DEVNULL,
                        stderr=subprocess.DEVNULL,
                    )
                except subprocess.CalledProcessError:
                    pass

    def __clean(self) -> None:
        """
        Clean the build and output directories.
        """
        self.logger.info("Cleaning build and output directories.")

        if os.path.exists(self.temp_build_output_dir):
            self.__remove_directory(self.temp_build_output_dir)
            self.logger.info(
                "Removed temporary build output directory: %s",
                self.temp_build_output_dir,
            )

        if os.path.exists(self.temp_binary_output_dir):
            self.__remove_directory(self.temp_binary_output_dir)
            self.logger.info(
                "Removed temporary binary output directory: %s",
                self.temp_binary_output_dir,
            )

        if os.path.exists(self.results_output_dir):
            self.__remove_directory(self.results_output_dir)
            self.logger.info(
                "Removed results output directory: %s", self.results_output_dir
            )

    def __configure(self) -> None:
        """
        Configure the project using CMake.
        """
        self.logger.info("Starting the CMake configuration.")

        cmake_command = [
            self.cmake,
            "-S",
            self.workspace,
            "-B",
            self.temp_build_output_dir,
            "-DCMAKE_BUILD_TYPE=Release",
            "-DIS_COMPILING_STATIC=true",
            "-DIS_COMPILING_SHARED=false",
            f"-DSOKKETTER_TARGET_ARCHITECTURE:STRING={self.architecture.value}",
            f"-DQt6_DIR={self.qt_cmake_folder}",
        ]

        if self.system == System.WINDOWS:
            cmake_generator = os.environ.get("CMAKE_GENERATOR")
            if not cmake_generator:
                desired_generator = self.__get_cmake_generator()

                cmake_command.extend(["-G", desired_generator])

                if "Visual Studio" in desired_generator:
                    if self.architecture == Architecture.X86_64:
                        cmake_command.extend(["-A", "x64"])
                    elif self.architecture == Architecture.ARM64:
                        cmake_command.extend(["-A", "ARM64"])

            # Do not force CMAKE_CXX_COMPILER on Windows; Visual Studio generators
            # resolve MSVC correctly even when cl.exe is not on PATH.
            cmake_command.extend(["-U", "CMAKE_CXX_COMPILER"])
        else:
            cmake_command.append(f"-DCMAKE_CXX_COMPILER={self.compiler}")
            cmake_command.append("-DCMAKE_GENERATOR:STRING=Ninja")

        cmake_prefix_path = os.environ.get("CMAKE_PREFIX_PATH")
        if cmake_prefix_path:
            merged_prefix_path = os.pathsep.join(
                [self.qt_root_folder, cmake_prefix_path]
            )
            cmake_command.append(f"-DCMAKE_PREFIX_PATH={merged_prefix_path}")
        else:
            cmake_command.append(f"-DCMAKE_PREFIX_PATH={self.qt_root_folder}")

        if BuildStage.TEST.value in self.stages and self.system != System.WINDOWS:
            cmake_command.append("-DSOKKETTER_ENABLE_TESTING=true")

        self.__execute_command(cmake_command)

        self.logger.info("CMake configuration completed successfully.")

    def __build(self) -> None:
        """
        Build the project using CMake.
        """
        self.logger.info("Starting the build process.")

        build_command = [
            self.cmake,
            "--build",
            self.temp_build_output_dir,
            "--config",
            "Release",
            "-j",
        ]
        self.__execute_command(build_command)

        self.logger.info("Build completed successfully.")

    def __verify_build(self) -> None:
        """
        Verify that the build was successful.
        """
        if not os.path.exists(self.temp_binary_output_dir):
            self.logger.error(
                "Build output directory does not exist: %s", self.temp_binary_output_dir
            )
            raise FileNotFoundError("Build output directory does not exist.")

        bin_folder = os.path.join(self.temp_binary_output_dir, "bin")
        if not os.path.exists(bin_folder):
            self.logger.error("Binary output directory does not exist: %s", bin_folder)
            raise FileNotFoundError("Binary output directory does not exist.")

        libs_folder = os.path.join(self.temp_binary_output_dir, "libs")
        if not os.path.exists(libs_folder):
            self.logger.error(
                "Library output directory does not exist: %s", libs_folder
            )
            raise FileNotFoundError("Library output directory does not exist.")

        include_folder = os.path.join(self.temp_binary_output_dir, "include")
        if not os.path.exists(include_folder):
            self.logger.error(
                "Include output directory does not exist: %s", include_folder
            )
            raise FileNotFoundError("Include output directory does not exist.")

        bin_files = glob.glob(os.path.join(bin_folder, "*"))
        if not bin_files:
            self.logger.error("No binary files found in bin directory: %s", bin_folder)
            raise FileNotFoundError("No binary files found in bin directory.")

        include_files = glob.glob(os.path.join(include_folder, "*.h"))
        if not include_files:
            self.logger.error(
                "No header files found in include directory: %s", include_folder
            )
            raise FileNotFoundError("No header files found in include directory.")

        lib_files = glob.glob(os.path.join(libs_folder, "*.a")) + glob.glob(
            os.path.join(libs_folder, "*.lib")
        )
        if not lib_files:
            self.logger.error(
                "No library files found in libs directory: %s", libs_folder
            )
            raise FileNotFoundError("No library files found in libs directory.")

        self.logger.info("Build verification completed successfully.")

    def __package_library(self) -> None:
        """
        Package the built library files.
        """
        self.logger.info("Starting the packaging of library files.")

        sokketter_lib_folder = os.path.join(self.results_output_dir, "libsokketter")
        if os.path.exists(sokketter_lib_folder):
            shutil.rmtree(sokketter_lib_folder)

        os.makedirs(sokketter_lib_folder)

        shutil.copytree(
            os.path.join(self.temp_binary_output_dir, "libs"),
            os.path.join(sokketter_lib_folder, "libs"),
        )

        shutil.copytree(
            os.path.join(self.temp_binary_output_dir, "include"),
            os.path.join(sokketter_lib_folder, "include"),
        )

        self.logger.info("Library files packaged successfully.")

    def __package_cli(self) -> None:
        """
        Package the CLI files.
        """
        self.logger.info("Starting the packaging of CLI files.")

        sokketter_cli_folder = os.path.join(self.results_output_dir, "sokketter-cli")
        if os.path.exists(sokketter_cli_folder):
            shutil.rmtree(sokketter_cli_folder)

        os.makedirs(sokketter_cli_folder)

        sokketter_cli_zip_folder = os.path.join(
            self.temp_binary_output_dir, "sokketter-cli-zipped"
        )
        if os.path.exists(sokketter_cli_zip_folder):
            shutil.rmtree(sokketter_cli_zip_folder)

        os.makedirs(sokketter_cli_zip_folder)

        if self.system == System.WINDOWS:
            shutil.copy(
                os.path.join(self.temp_binary_output_dir, "bin", "sokketter-cli.exe"),
                sokketter_cli_zip_folder,
            )
        else:
            shutil.copy(
                os.path.join(self.temp_binary_output_dir, "bin", "sokketter-cli"),
                sokketter_cli_zip_folder,
            )

        zip_name = shutil.make_archive(
            base_name=f"sokketter-cli-{self.version}-{self.system.value}-{self.os_version}-{self.architecture.value}",
            format="zip",
            root_dir=sokketter_cli_zip_folder,
        )

        shutil.move(
            src=os.path.join(self.workspace, zip_name),
            dst=sokketter_cli_folder,
        )

        self.logger.info("CLI files packaged successfully.")

    def __compute_linux_deb_depends(self, binary_path: str) -> str:
        """
        Derive a versioned Depends field from the built binary.

        dpkg-shlibdeps resolves minimum versions for shared libraries owned by apt
        packages (e.g. libc6, libstdc++6, libudev1). Qt6 libraries are bundled
        privately with the package (see __bundle_qt_runtime_for_deb), so they are not
        owned by any distro package; --ignore-missing-info makes dpkg-shlibdeps skip
        them instead of failing or guessing at a package name.
        """
        scratch_dir = os.path.join(
            self.temp_binary_output_dir, "sokketter-ui-shlibdeps"
        )
        if os.path.exists(scratch_dir):
            shutil.rmtree(scratch_dir)

        debian_dir = os.path.join(scratch_dir, "debian")
        os.makedirs(debian_dir)

        with open(
            file=os.path.join(debian_dir, "control"), mode="w", encoding="utf-8"
        ) as file:
            file.write(
                "Source: sokketter-ui\n"
                "Priority: optional\n"
                "Maintainer: Paul Ergard <64430090+morwy@users.noreply.github.com>\n"
                "\n"
                "Package: sokketter-ui\n"
                "Architecture: any\n"
                "Depends: ${shlibs:Depends}\n"
                "Description: UI application for controlling connected power strips and sockets.\n"
            )

        try:
            result = subprocess.run(
                ["dpkg-shlibdeps", "-O", "--ignore-missing-info", "-e", binary_path],
                check=True,
                capture_output=True,
                text=True,
                cwd=scratch_dir,
            )
        finally:
            shutil.rmtree(scratch_dir)

        shlibs_depends = ""
        for line in result.stdout.splitlines():
            if line.startswith("shlibs:Depends="):
                shlibs_depends = line.split("=", maxsplit=1)[1].strip()
                break

        return shlibs_depends

    def __bundle_qt_runtime_for_deb(
        self, usr_bin_folder: str, deb_root_folder: str
    ) -> None:
        """
        Deploy a private copy of the linked Qt runtime under usr/lib/sokketter-ui so the
        Debian package does not depend on the target distro's Qt6 packages, whose names
        and available versions vary across supported releases.
        """
        self.logger.info("Bundling a private Qt runtime for the Debian package.")

        # linuxdeployqt looks for this file due to a known upstream glibc-version check bug.
        linuxdeployqt_idiot_fix_folder_path = os.path.join(
            deb_root_folder, "usr", "share", "doc", "libc6"
        )
        os.makedirs(linuxdeployqt_idiot_fix_folder_path, exist_ok=True)
        pathlib.Path(
            os.path.join(linuxdeployqt_idiot_fix_folder_path, "copyright")
        ).touch()

        linuxdeployqt_path = os.path.join(
            self.workspace,
            f"linuxdeployqt-continuous-{self.__get_linuxdeployqt_architecture()}.AppImage",
        )

        binary_path = os.path.join(usr_bin_folder, "sokketter-ui")
        deploy_command = [
            linuxdeployqt_path,
            binary_path,
            "-verbose=2",
            "-unsupported-allow-new-glibc",
            "-qmake=" + self.__resolve_qt_tool("qmake"),
        ]
        self.__execute_command(cmake_command=deploy_command, cwd=deb_root_folder)

        # The workaround file is owned by the real libc6 package; strip it before packaging.
        shutil.rmtree(os.path.join(deb_root_folder, "usr", "share", "doc", "libc6"))

        # linuxdeployqt deploys Qt into usr/lib and usr/plugins (RPATH: $ORIGIN/../lib).
        # Relocate that into a private subfolder so it cannot collide with system Qt.
        deployed_lib_folder = os.path.join(deb_root_folder, "usr", "lib")
        deployed_plugins_folder = os.path.join(deb_root_folder, "usr", "plugins")

        staging_folder = os.path.join(deb_root_folder, "usr", "lib-staging")
        shutil.move(deployed_lib_folder, staging_folder)
        os.makedirs(deployed_lib_folder)
        private_lib_folder = os.path.join(deployed_lib_folder, "sokketter-ui")
        shutil.move(staging_folder, private_lib_folder)

        private_bin_folder = os.path.join(private_lib_folder, "bin")

        if os.path.exists(deployed_plugins_folder):
            shutil.move(
                deployed_plugins_folder, os.path.join(private_lib_folder, "plugins")
            )

        for root, dirs, files in os.walk(private_lib_folder):
            for directory in dirs:
                os.chmod(os.path.join(root, directory), 0o755)
            for filename in files:
                os.chmod(os.path.join(root, filename), 0o644)

        os.makedirs(private_bin_folder)
        binary_path = os.path.join(private_bin_folder, "sokketter-ui")
        shutil.move(os.path.join(usr_bin_folder, "sokketter-ui"), binary_path)
        os.chmod(binary_path, 0o755)

        # Only the executable's RPATH needs adjusting; libraries reference each other
        # via $ORIGIN, which is unaffected by the extra sokketter-ui path segment.
        self.__execute_command(["patchelf", "--set-rpath", "$ORIGIN/..", binary_path])

        qt_conf_path = os.path.join(private_bin_folder, "qt.conf")
        with open(file=qt_conf_path, mode="w", encoding="utf-8") as file:
            file.write("[Paths]\nPrefix = ..\nPlugins = plugins\n")
        os.chmod(qt_conf_path, 0o644)

        self.logger.info("Private Qt runtime bundled successfully.")

    def __package_linux_ui_deb(self) -> None:
        """
        Package the Linux UI application as a Debian package.
        """
        self.logger.info("Starting the packaging of Linux UI Debian package.")

        package_name = "sokketter-ui"
        package_version = self.version
        package_architecture = self.__get_debian_architecture()
        deb_root_folder = os.path.join(
            self.temp_binary_output_dir, f"{package_name}-deb-root"
        )

        if os.path.exists(deb_root_folder):
            shutil.rmtree(deb_root_folder)

        debian_folder = os.path.join(deb_root_folder, "DEBIAN")
        usr_bin_folder = os.path.join(deb_root_folder, "usr", "bin")
        applications_folder = os.path.join(
            deb_root_folder, "usr", "share", "applications"
        )
        icons_folder = os.path.join(
            deb_root_folder, "usr", "share", "icons", "hicolor", "256x256", "apps"
        )
        udev_rules_folder = os.path.join(deb_root_folder, "lib", "udev", "rules.d")

        for folder in [
            debian_folder,
            usr_bin_folder,
            applications_folder,
            icons_folder,
            udev_rules_folder,
        ]:
            os.makedirs(folder, exist_ok=True)
            os.chmod(folder, 0o755)

        shutil.copy(
            os.path.join(self.temp_binary_output_dir, "bin", "sokketter-ui"),
            os.path.join(usr_bin_folder, "sokketter-ui"),
        )
        os.chmod(os.path.join(usr_bin_folder, "sokketter-ui"), 0o755)

        desktop_file_path = os.path.join(applications_folder, "sokketter-ui.desktop")
        shutil.copy(
            os.path.join(
                self.workspace, "sokketter-ui", "resources", "sokketter-ui.desktop"
            ),
            desktop_file_path,
        )
        os.chmod(desktop_file_path, 0o644)
        with open(file=desktop_file_path, mode="r", encoding="utf-8") as file:
            desktop_file_lines = file.readlines()

        with open(file=desktop_file_path, mode="w", encoding="utf-8") as file:
            for line in desktop_file_lines:
                if line.startswith("Exec="):
                    file.write("Exec=/usr/bin/sokketter-ui %u\n")
                elif line.startswith("X-AppImage-Version="):
                    file.write(f"X-AppImage-Version={self.version}\n")
                elif line.startswith("X-AppImage-Arch="):
                    file.write(f"X-AppImage-Arch={self.architecture.value}\n")
                else:
                    file.write(line)

        shutil.copy(
            os.path.join(
                self.workspace,
                "sokketter-ui",
                "resources",
                "icons",
                "socket-icon-256x256.png",
            ),
            os.path.join(icons_folder, "sokketter-ui-icon.png"),
        )
        os.chmod(os.path.join(icons_folder, "sokketter-ui-icon.png"), 0o644)

        shutil.copy(
            os.path.join(self.workspace, "udev-rules", "101-sokketter.rules"),
            os.path.join(udev_rules_folder, "101-sokketter.rules"),
        )
        os.chmod(os.path.join(udev_rules_folder, "101-sokketter.rules"), 0o644)

        self.__bundle_qt_runtime_for_deb(usr_bin_folder, deb_root_folder)

        launcher_path = os.path.join(usr_bin_folder, "sokketter-ui")
        with open(file=launcher_path, mode="w", encoding="utf-8") as file:
            file.write('#!/bin/sh\nexec /usr/lib/sokketter-ui/bin/sokketter-ui "$@"\n')
        os.chmod(launcher_path, 0o755)

        package_depends = self.__compute_linux_deb_depends(
            os.path.join(
                deb_root_folder, "usr", "lib", package_name, "bin", "sokketter-ui"
            )
        )

        depends_line = f"Depends: {package_depends}\n" if package_depends else ""
        control_content = f"""Package: {package_name}
Version: {package_version}
Section: utils
Priority: optional
Architecture: {package_architecture}
Maintainer: Paul Ergard <64430090+morwy@users.noreply.github.com>
{depends_line}Description: UI application for controlling connected power strips and sockets.
 sokketter-ui provides a Qt-based desktop interface for supported USB and Ethernet power strips.
"""
        with open(
            file=os.path.join(debian_folder, "control"), mode="w", encoding="utf-8"
        ) as file:
            file.write(control_content)

        postinst_content = """#!/bin/sh
set -e

if command -v udevadm >/dev/null 2>&1; then
    udevadm control --reload-rules || true
    udevadm trigger || true
fi

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database /usr/share/applications || true
fi

if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q /usr/share/icons/hicolor || true
fi

exit 0
"""
        postrm_content = """#!/bin/sh
set -e

if command -v udevadm >/dev/null 2>&1; then
    udevadm control --reload-rules || true
    udevadm trigger || true
fi

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database /usr/share/applications || true
fi

if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q /usr/share/icons/hicolor || true
fi

exit 0
"""

        for script_name, script_content in [
            ("postinst", postinst_content),
            ("postrm", postrm_content),
        ]:
            script_path = os.path.join(debian_folder, script_name)
            with open(file=script_path, mode="w", encoding="utf-8") as file:
                file.write(script_content)
            os.chmod(script_path, 0o755)

        deb_filename = (
            f"{package_name}-{package_version}-{self.system.value}-{self.os_version}-"
            f"{self.architecture.value}.deb"
        )
        deb_output_path = os.path.join(
            self.results_output_dir, "sokketter-ui", deb_filename
        )
        packing_command = [
            "dpkg-deb",
            "--build",
            "--root-owner-group",
            deb_root_folder,
            deb_output_path,
        ]
        self.__execute_command(packing_command)

        self.__validate_deb_package(deb_output_path)

        self.logger.info("Linux UI Debian package packaged successfully.")

    def __validate_deb_package(self, deb_path: str) -> None:
        """
        Validate the Debian package archive and its contents.
        """
        if not os.path.isfile(deb_path):
            raise FileNotFoundError(f"Debian package was not created: {deb_path}")

        self.logger.info("Validating Linux UI Debian package: %s", deb_path)
        self.__execute_command(["dpkg-deb", "--info", deb_path])
        self.__execute_command(["dpkg-deb", "--contents", deb_path])
        self.logger.info("Linux UI Debian package validation completed successfully.")

    def __package_linux_app_image(self, sokketter_ui_folder, sokketter_ui_zip_folder):
        sokketter_app_image_folder = os.path.join(
            self.temp_binary_output_dir, "sokketter-ui.AppImage"
        )
        os.makedirs(sokketter_app_image_folder, exist_ok=True)

        usr_bin_folder = os.path.join(sokketter_app_image_folder, "usr", "bin")
        os.makedirs(usr_bin_folder, exist_ok=True)

        shutil.copy(
            os.path.join(self.temp_binary_output_dir, "bin", "sokketter-ui"),
            usr_bin_folder,
        )

        shutil.copy(
            os.path.join(
                self.workspace, "sokketter-ui", "resources", "sokketter-ui.desktop"
            ),
            sokketter_app_image_folder,
        )

        shutil.copy(
            os.path.join(
                self.workspace,
                "sokketter-ui",
                "resources",
                "icons",
                "socket-icon-256x256.png",
            ),
            os.path.join(sokketter_app_image_folder, "sokketter-ui-icon.png"),
        )

        desktop_file_path = os.path.join(
            sokketter_app_image_folder, "sokketter-ui.desktop"
        )
        with open(file=desktop_file_path, mode="r", encoding="utf-8") as file:
            desktop_file_lines = file.readlines()

        with open(file=desktop_file_path, mode="w", encoding="utf-8") as file:
            for line in desktop_file_lines:
                if line.startswith("X-AppImage-Version="):
                    file.write(f"X-AppImage-Version={self.version}\n")
                else:
                    file.write(line)

        linuxdeployqt_idiot_fix_folder_path = os.path.join(
            sokketter_app_image_folder, "usr", "share", "doc", "libc6"
        )
        os.makedirs(linuxdeployqt_idiot_fix_folder_path, exist_ok=True)
        linuxdeployqt_idiot_fix_path = os.path.join(
            linuxdeployqt_idiot_fix_folder_path, "copyright"
        )
        pathlib.Path(linuxdeployqt_idiot_fix_path).touch()

        linuxdeployqt_path = os.path.join(
            self.workspace,
            f"linuxdeployqt-continuous-{self.__get_linuxdeployqt_architecture()}.AppImage",
        )

        packing_command = [
            linuxdeployqt_path,
            os.path.join(usr_bin_folder, "sokketter-ui"),
            "-appimage",
            f"-executable={os.path.join(usr_bin_folder, 'sokketter-ui')}",
            "-verbose=2",
            "-unsupported-allow-new-glibc",
            "-qmake=" + self.__resolve_qt_tool("qmake"),
        ]
        self.__execute_command(
            cmake_command=packing_command, cwd=sokketter_ui_zip_folder
        )

        appimage_pattern = os.path.join(
            sokketter_ui_zip_folder, "sokketter-ui-*.AppImage"
        )
        appimage_files = glob.glob(appimage_pattern)

        for appimage_file in appimage_files:
            new_appimage_path = os.path.join(
                sokketter_ui_zip_folder, "sokketter-ui.AppImage"
            )
            os.rename(appimage_file, new_appimage_path)
            self.logger.info("Renamed %s to %s", appimage_file, new_appimage_path)
            break

        zip_name = shutil.make_archive(
            base_name=f"sokketter-ui-{self.version}-{self.system.value}-{self.os_version}-{self.architecture.value}",
            format="zip",
            root_dir=sokketter_ui_zip_folder,
        )

        shutil.move(
            src=os.path.join(self.workspace, zip_name),
            dst=sokketter_ui_folder,
        )

    def __package_ui(self) -> None:
        """
        Package the UI files.
        """
        self.logger.info("Starting the packaging of UI files.")

        sokketter_ui_folder = os.path.join(self.results_output_dir, "sokketter-ui")
        if os.path.exists(sokketter_ui_folder):
            shutil.rmtree(sokketter_ui_folder)

        os.makedirs(sokketter_ui_folder)

        sokketter_ui_zip_folder = os.path.join(
            self.temp_binary_output_dir, "sokketter-ui-zipped"
        )
        if os.path.exists(sokketter_ui_zip_folder):
            shutil.rmtree(sokketter_ui_zip_folder)

        os.makedirs(sokketter_ui_zip_folder)

        if self.system == System.WINDOWS:
            shutil.copy(
                os.path.join(self.temp_binary_output_dir, "bin", "sokketter-ui.exe"),
                sokketter_ui_zip_folder,
            )
            packing_command = [
                self.__resolve_qt_tool("windeployqt"),
                os.path.join(sokketter_ui_zip_folder, "sokketter-ui.exe"),
                sokketter_ui_zip_folder,
            ]
            self.__execute_command(packing_command)

            zip_name = shutil.make_archive(
                base_name=f"sokketter-ui-{self.version}-{self.system.value}-{self.os_version}-{self.architecture.value}",
                format="zip",
                root_dir=sokketter_ui_zip_folder,
            )

            shutil.move(
                src=os.path.join(self.workspace, zip_name),
                dst=sokketter_ui_folder,
            )

        elif self.system == System.MACOS:
            filename = "sokketter-ui.app"
            app_filepath = os.path.join(sokketter_ui_zip_folder, filename)

            shutil.copytree(
                src=os.path.join(self.temp_binary_output_dir, "bin", filename),
                dst=app_filepath,
                symlinks=True,
            )

            packing_command = [
                self.__resolve_qt_tool("macdeployqt"),
                app_filepath,
                "-verbose=2",
            ]
            self.__execute_command(packing_command)

            packing_command = [
                "codesign",
                "--force",
                "--deep",
                "--sign",
                "-",
                "--timestamp=none",
                app_filepath,
            ]
            self.__execute_command(packing_command)

            zip_name = os.path.join(
                self.workspace,
                f"sokketter-ui-{self.version}-{self.system.value}-{self.os_version}-{self.architecture.value}.zip",
            )

            packing_command = [
                "ditto",
                "-c",
                "-k",
                "--keepParent",
                "--sequesterRsrc",
                app_filepath,
                zip_name,
            ]
            self.__execute_command(packing_command)

            shutil.move(
                src=zip_name,
                dst=sokketter_ui_folder,
            )

            dmg_filename = os.path.join(
                sokketter_ui_folder,
                f"sokketter-ui-{self.version}-{self.system.value}-{self.os_version}-{self.architecture.value}.dmg",
            )

            self.__create_dmg(app_path=app_filepath, output_path=dmg_filename)

        elif self.system == System.LINUX:
            self.__package_linux_app_image(sokketter_ui_folder, sokketter_ui_zip_folder)
            self.__package_linux_ui_deb()

        self.logger.info("UI files packaged successfully.")

    def __package(self) -> None:
        """
        Package the built binaries.
        """
        self.logger.info("Starting the packaging process.")

        self.__package_library()
        self.__package_cli()
        self.__package_ui()

        self.logger.info("Packaging completed successfully.")

    def run(self) -> None:
        """
        Run the build process.
        """
        self.logger.info("Starting the build process.")

        if BuildStage.CLEAN.value in self.stages or BuildStage.ALL.value in self.stages:
            self.__clean()

        if (
            BuildStage.CONFIGURE.value in self.stages
            or BuildStage.ALL.value in self.stages
        ):
            self.__configure()

        if BuildStage.BUILD.value in self.stages or BuildStage.ALL.value in self.stages:
            self.__build()

        if (
            BuildStage.VERIFY.value in self.stages
            or BuildStage.ALL.value in self.stages
        ):
            self.__verify_build()

        if (
            BuildStage.PACKAGE.value in self.stages
            or BuildStage.ALL.value in self.stages
        ):
            self.__package()

        self.logger.info("Build process completed successfully.")


# --------------------------------------------------------------------------------------------------
#
# Entry point.
#
# --------------------------------------------------------------------------------------------------
if __name__ == "__main__":
    logging.basicConfig(
        level=logging.DEBUG,
        format="%(asctime)s [%(levelname)s] %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
        handlers=[
            logging.StreamHandler(sys.stdout),
            logging.FileHandler("build.log", mode="w"),
        ],
    )

    parser = argparse.ArgumentParser(
        description="Build script for the sokketter project."
    )

    parser.add_argument(
        "--stages",
        type=str,
        nargs="*",
        metavar="STAGES",
        default=[BuildStage.ALL],
        choices=[stage.value for stage in BuildStage],
        help=f"Stages to run (default: {BuildStage.ALL.name}). Available stages: {', '.join(stage.value for stage in BuildStage)}.",
    )

    parser.add_argument(
        "--qt-version",
        type=lambda value: (
            value
            if value == "latest" or re.fullmatch(r"\d+\.\d+(?:\.\d+)?", value)
            else parser.error(
                "argument --qt-version: must be 'latest' or a two-/three-component version"
            )
        ),
        default="latest",
        metavar="QT_VERSION",
        help="Qt version to use (default: latest; format: major.minor[.patch]).",
    )

    host_architecture = Environment.get_architecture()

    parser.add_argument(
        "--architecture",
        type=Architecture,
        choices=list(Architecture),
        default=host_architecture,
        metavar="ARCHITECTURE",
        help=(
            "Target architecture for the build "
            f"(default: current host architecture, which is currently {host_architecture.value})."
        ),
    )

    args = parser.parse_args()

    if not args.stages:
        print("No stages specified. Use --help to see available stages.")
        sys.exit(1)

    if args.architecture != host_architecture:
        parser.error(
            "Cross-compilation is not supported yet. "
            f"Requested --architecture={args.architecture.value}, "
            f"but host architecture is {host_architecture.value}."
        )

    Build(args.stages, args.qt_version, args.architecture).run()
