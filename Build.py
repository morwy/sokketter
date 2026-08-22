#!/usr/bin/env python

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
import platform
import shutil
import subprocess
import sys
import tempfile
import time
from enum import Enum

from Environment import Environment
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

    def __init__(self, stages: list[str]) -> None:
        """
        Initialize the build class.
        """
        logging.basicConfig(
            level=logging.INFO,
            format="%(asctime)s - %(levelname)s - %(message)s",
        )
        self.logger = logging.getLogger(__name__)
        self.logger.info("Build class initialized.")

        self.stages = stages
        self.logger.info("Build stages: %s", self.stages)

        self.cmake = self.__get_cmake()
        self.logger.info("CMake executable: %s", self.cmake)

        self.compiler = self.__get_cpp_compiler()
        self.logger.info("C++ compiler: %s", self.compiler)

        self.windows_msvc_env_script: str | None = None
        if platform.system() == "Windows":
            self.windows_msvc_env_script = self.__resolve_windows_msvc_env_script()
            if self.windows_msvc_env_script:
                self.logger.info(
                    "Using Visual Studio developer environment script: %s",
                    self.windows_msvc_env_script,
                )

        self.os_name = Environment.get_os_name()
        self.logger.info("Operating system: %s", self.os_name)

        self.os_version = Environment.get_os_version()
        self.logger.info("Operating system version: %s", self.os_version)

        self.architecture = Environment.get_architecture()
        self.logger.info("Architecture: %s", self.architecture)

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
        cmake = "cmake"
        cmake_in_path = shutil.which(cmake)
        if cmake_in_path:
            if platform.system() == "Windows" and any(
                marker in cmake_in_path.lower()
                for marker in ["mingw", "msys", "cygwin"]
            ):
                self.logger.warning(
                    "Detected MSYS/MinGW CMake on PATH (%s). Looking for native Windows CMake instead.",
                    cmake_in_path,
                )
            else:
                return cmake

        self.logger.warning(
            "Using a fallback CMake resolution path instead of the default PATH entry."
        )

        if platform.system() == "Windows":
            windows_cmake_candidates = [
                "C:\\Qt\\Tools\\CMake_64\\bin\\cmake.exe",
                "C:\\Program Files\\CMake\\bin\\cmake.exe",
            ]

            for candidate in windows_cmake_candidates:
                if os.path.exists(candidate):
                    return candidate

            if cmake_in_path:
                return cmake_in_path

            cmake = windows_cmake_candidates[0]
        elif platform.system() in ["Linux", "Darwin"]:
            cmake = os.path.join(
                os.environ.get("HOME", ""), "Qt", "Tools", "CMake", "bin", "cmake"
            )

        if not os.path.exists(cmake):
            raise EnvironmentError("CMake executable not found")

        return cmake

    def __get_cpp_compiler(self) -> str:
        """
        Get the C++ compiler from the environment variable.
        """
        compiler = ""

        if platform.system() == "Windows":
            compiler = "cl"
        elif platform.system() == "Linux":
            compiler = "g++"
        elif platform.system() == "Darwin":
            compiler = "clang++"
        else:
            self.logger.error("Unsupported platform: %s", platform.system())
            raise EnvironmentError("Unsupported platform")

        return compiler

    def __construct_binary_output_dir(self, workspace: str) -> str:
        """
        Get the binary output directory based on the platform.
        """
        if platform.system() == "Windows":
            return os.path.join(
                workspace, "bin", f"windows_{self.architecture}", "Release"
            )

        if platform.system() == "Linux":
            return os.path.join(
                workspace, "bin", f"linux_{self.architecture}", "Release"
            )

        if platform.system() == "Darwin":
            return os.path.join(
                workspace, "bin", f"macos_{self.architecture}", "Release"
            )

        self.logger.error("Unsupported platform: %s", platform.system())
        raise EnvironmentError("Unsupported platform")

    def __resolve_qt6_dir(self) -> str:
        """
        Resolve Qt6_DIR to a path containing Qt6Config.cmake across supported platforms.
        """

        def has_qt6_config(path: pathlib.Path) -> bool:
            return (path / "Qt6Config.cmake").exists() or (
                path / "qt6-config.cmake"
            ).exists()

        def qt6_dirs_from_prefix(prefix: pathlib.Path) -> list[pathlib.Path]:
            return [prefix, prefix / "lib" / "cmake" / "Qt6"]

        def is_qt_dir_arch_compatible(path: pathlib.Path) -> bool:
            if platform.system() != "Windows":
                return True

            normalized = str(path).lower()
            arch = self.architecture.lower()

            if arch in ["x86_64", "amd64"]:
                return "arm64" not in normalized

            if arch in ["arm64", "aarch64"]:
                return "arm64" in normalized

            return True

        def discover_windows_qt6_dir_for_arch() -> str | None:
            if platform.system() != "Windows":
                return None

            arch = self.architecture.lower()
            patterns: list[str]
            if arch in ["x86_64", "amd64"]:
                patterns = [
                    os.path.join("C:\\Qt", "*", "msvc*_64", "lib", "cmake", "Qt6"),
                    os.path.join("C:\\Qt", "*", "mingw*_64", "lib", "cmake", "Qt6"),
                    os.path.join(
                        user_profile, "Qt", "*", "msvc*_64", "lib", "cmake", "Qt6"
                    ),
                    os.path.join(
                        user_profile, "Qt", "*", "mingw*_64", "lib", "cmake", "Qt6"
                    ),
                ]
            elif arch in ["arm64", "aarch64"]:
                patterns = [
                    os.path.join("C:\\Qt", "*", "*arm64*", "lib", "cmake", "Qt6"),
                    os.path.join(
                        user_profile, "Qt", "*", "*arm64*", "lib", "cmake", "Qt6"
                    ),
                ]
            else:
                patterns = []

            discovered: list[pathlib.Path] = []
            for pattern in patterns:
                for match in sorted(glob.glob(pattern), reverse=True):
                    discovered.append(pathlib.Path(match).resolve())

            for candidate in discovered:
                if has_qt6_config(candidate):
                    return str(candidate)

            return None

        qt6_dir = os.environ.get("Qt6_DIR") or os.environ.get("QT6_DIR")
        if qt6_dir:
            qt6_path = pathlib.Path(qt6_dir).expanduser().resolve()
            if has_qt6_config(qt6_path):
                if not is_qt_dir_arch_compatible(qt6_path):
                    fallback_qt6_dir = discover_windows_qt6_dir_for_arch()
                    if fallback_qt6_dir:
                        self.logger.warning(
                            "Qt6_DIR '%s' does not match build architecture '%s'. Using '%s' instead.",
                            qt6_path,
                            self.architecture,
                            fallback_qt6_dir,
                        )
                        return fallback_qt6_dir
                return str(qt6_path)
            raise EnvironmentError(
                f"Qt6_DIR is set to '{qt6_path}' but Qt6Config.cmake was not found there."
            )

        candidates: list[pathlib.Path] = []
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
        user_profile = os.environ.get("USERPROFILE", "")

        if platform.system() == "Darwin":
            patterns = [
                os.path.join(home_dir, "Qt", "*", "macos", "lib", "cmake", "Qt6"),
            ]
        elif platform.system() == "Linux":
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
        elif platform.system() == "Windows":
            patterns = [
                os.path.join("C:\\Qt", "*", "msvc*", "lib", "cmake", "Qt6"),
                os.path.join("C:\\Qt", "*", "mingw*", "lib", "cmake", "Qt6"),
                os.path.join(user_profile, "Qt", "*", "msvc*", "lib", "cmake", "Qt6"),
                os.path.join(user_profile, "Qt", "*", "mingw*", "lib", "cmake", "Qt6"),
            ]
        else:
            patterns = []

        for pattern in patterns:
            for match in sorted(glob.glob(pattern), reverse=True):
                candidates.append(pathlib.Path(match).resolve())

        for candidate in candidates:
            if has_qt6_config(candidate) and is_qt_dir_arch_compatible(candidate):
                return str(candidate)

        if platform.system() == "Windows":
            fallback_qt6_dir = discover_windows_qt6_dir_for_arch()
            if fallback_qt6_dir:
                return fallback_qt6_dir

        raise EnvironmentError(
            "Qt6_DIR is not set and Qt6 could not be auto-discovered for this platform. "
            "Set Qt6_DIR (or QT6_DIR) to a directory containing Qt6Config.cmake."
        )

    def __resolve_qt_tool(self, tool_name: str) -> str:
        """
        Resolve Qt deployment tools (e.g. macdeployqt, windeployqt) to an executable path.
        """
        tool_in_path = shutil.which(tool_name)
        if tool_in_path:
            return tool_in_path

        executable_name = tool_name
        if platform.system() == "Windows" and not tool_name.endswith(".exe"):
            executable_name = f"{tool_name}.exe"

        candidates: list[pathlib.Path] = []

        qt6_dir = os.environ.get("Qt6_DIR") or os.environ.get("QT6_DIR")
        if qt6_dir:
            qt6_path = pathlib.Path(qt6_dir).expanduser().resolve()
            # Qt6_DIR usually points to <qt-root>/lib/cmake/Qt6, so go to <qt-root>/bin.
            qt_root_candidate = qt6_path.parent.parent.parent
            candidates.append(qt_root_candidate / "bin" / executable_name)

        home_dir = os.environ.get("HOME", "")
        if platform.system() == "Darwin":
            for path in glob.glob(
                os.path.join(home_dir, "Qt", "*", "macos", "bin", executable_name)
            ):
                candidates.append(pathlib.Path(path))
        elif platform.system() == "Linux":
            for path in glob.glob(
                os.path.join(home_dir, "Qt", "*", "gcc_64", "bin", executable_name)
            ):
                candidates.append(pathlib.Path(path))
        elif platform.system() == "Windows":
            for path in glob.glob(
                os.path.join("C:\\Qt", "*", "*", "bin", executable_name)
            ):
                candidates.append(pathlib.Path(path))

        for candidate in candidates:
            if candidate.exists():
                return str(candidate)

        raise FileNotFoundError(
            f"Could not find '{tool_name}'. Add Qt's bin directory to PATH or set Qt6_DIR/QT6_DIR."
        )

    def __resolve_windows_msvc_env_script(self) -> str | None:
        """
        Resolve a Visual Studio developer environment batch script path.
        """
        if platform.system() != "Windows":
            return None

        vswhere = os.path.join(
            os.environ.get("ProgramFiles(x86)", "C:\\Program Files (x86)"),
            "Microsoft Visual Studio",
            "Installer",
            "vswhere.exe",
        )

        if os.path.exists(vswhere):
            script_patterns = [
                "Common7\\Tools\\VsDevCmd.bat",
                "VC\\Auxiliary\\Build\\vcvars64.bat",
                "VC\\Auxiliary\\Build\\vcvarsall.bat",
            ]

            for pattern in script_patterns:
                result = subprocess.run(
                    [
                        vswhere,
                        "-latest",
                        "-products",
                        "*",
                        "-requires",
                        "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
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
            os.path.join("VC", "Auxiliary", "Build", "vcvars64.bat"),
            os.path.join("VC", "Auxiliary", "Build", "vcvarsall.bat"),
        ]

        for root in fallback_roots:
            for rel_path in fallback_paths:
                candidate = os.path.join(root, rel_path)
                if os.path.exists(candidate):
                    return candidate

        return None

    def __create_windows_msvc_wrapper_command(
        self, command: list[str]
    ) -> tuple[list[str], str]:
        """
        Create a temporary .cmd wrapper that initializes VS dev env and executes command.
        """
        if self.windows_msvc_env_script is None:
            raise RuntimeError("Visual Studio environment script is not available.")

        script_name = os.path.basename(self.windows_msvc_env_script).lower()
        if script_name == "vsdevcmd.bat":
            vcvars_call = (
                f'call "{self.windows_msvc_env_script}" -arch=x64 -host_arch=x64 >nul'
            )
        elif script_name == "vcvarsall.bat":
            vcvars_call = f'call "{self.windows_msvc_env_script}" x64 >nul'
        else:
            vcvars_call = f'call "{self.windows_msvc_env_script}" >nul'

        command_line = subprocess.list2cmdline(command)
        wrapper_content = (
            "@echo off\n"
            f"{vcvars_call}\n"
            "if errorlevel 1 exit /b %errorlevel%\n"
            f"{command_line}\n"
            "exit /b %errorlevel%\n"
        )

        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".cmd", delete=False, encoding="utf-8"
        ) as wrapper_file:
            wrapper_file.write(wrapper_content)
            wrapper_path = wrapper_file.name

        return ["cmd.exe", "/c", wrapper_path], wrapper_path

    def __execute_command(self, cmake_command, cwd: str | None = None):
        wrapper_path: str | None = None
        try:
            command_to_run = cmake_command

            if (
                platform.system() == "Windows"
                and isinstance(command_to_run, list)
                and self.windows_msvc_env_script is not None
            ):
                command_to_run, wrapper_path = (
                    self.__create_windows_msvc_wrapper_command(command_to_run)
                )

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
                shell=isinstance(command_to_run, str),
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

        finally:
            if wrapper_path and os.path.exists(wrapper_path):
                os.remove(wrapper_path)

    def __get_cached_cmake_generator(self, build_dir: str) -> str | None:
        """
        Read the CMake generator recorded in CMakeCache.txt, if available.
        """
        cache_file = os.path.join(build_dir, "CMakeCache.txt")
        if not os.path.exists(cache_file):
            return None

        with open(cache_file, "r", encoding="utf-8", errors="ignore") as file:
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

    def __get_cmake_generator(self, qt6_dir):
        desired_generator = "Visual Studio 17 2022"
        qt6_dir_lower = qt6_dir.lower()
        if "msvc2019" in qt6_dir_lower:
            desired_generator = "Visual Studio 16 2019"
        elif "msvc2022" in qt6_dir_lower:
            desired_generator = "Visual Studio 17 2022"

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

    def __clean(self) -> None:
        """
        Clean the build and output directories.
        """
        self.logger.info("Cleaning build and output directories.")

        if os.path.exists(self.temp_build_output_dir):
            shutil.rmtree(self.temp_build_output_dir)
            self.logger.info(
                "Removed temporary build output directory: %s",
                self.temp_build_output_dir,
            )

        if os.path.exists(self.temp_binary_output_dir):
            shutil.rmtree(self.temp_binary_output_dir)
            self.logger.info(
                "Removed temporary binary output directory: %s",
                self.temp_binary_output_dir,
            )

        if os.path.exists(self.results_output_dir):
            shutil.rmtree(self.results_output_dir)
            self.logger.info(
                "Removed results output directory: %s", self.results_output_dir
            )

    def __configure(self) -> None:
        """
        Configure the project using CMake.
        """
        self.logger.info("Starting the CMake configuration.")

        qt6_dir = self.__resolve_qt6_dir()
        qt6_root = str(pathlib.Path(qt6_dir).parent.parent.parent)

        cmake_command = [
            self.cmake,
            "-S",
            self.workspace,
            "-B",
            self.temp_build_output_dir,
            "-DIS_COMPILING_STATIC=true",
            "-DIS_COMPILING_SHARED=false",
            f"-DQt6_DIR={qt6_dir}",
        ]

        if platform.system() == "Windows":
            cmake_generator = os.environ.get("CMAKE_GENERATOR")
            if not cmake_generator:
                desired_generator = self.__get_cmake_generator(qt6_dir)

                cmake_command.extend(["-G", desired_generator])
                if self.architecture.lower() in ["x86_64", "amd64"]:
                    cmake_command.extend(["-A", "x64"])
                elif self.architecture.lower() in ["arm64", "aarch64"]:
                    cmake_command.extend(["-A", "ARM64"])

            # Do not force CMAKE_CXX_COMPILER on Windows; Visual Studio generators
            # resolve MSVC correctly even when cl.exe is not on PATH.
            cmake_command.extend(["-U", "CMAKE_CXX_COMPILER"])
        else:
            cmake_command.append(f"-DCMAKE_CXX_COMPILER={self.compiler}")

        cmake_prefix_path = os.environ.get("CMAKE_PREFIX_PATH")
        if cmake_prefix_path:
            merged_prefix_path = os.pathsep.join([qt6_root, cmake_prefix_path])
            cmake_command.append(f"-DCMAKE_PREFIX_PATH={merged_prefix_path}")
        else:
            cmake_command.append(f"-DCMAKE_PREFIX_PATH={qt6_root}")

        if BuildStage.TEST.value in self.stages and platform.system() != "Windows":
            cmake_command.append("-DSOKKETTER_ENABLE_TESTING=true")

        self.__execute_command(cmake_command)

        self.logger.info("CMake configuration completed successfully.")

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

        if platform.system() == "Windows":
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
            base_name=f"sokketter-cli-{self.version}-{self.os_name}-{self.os_version}-{self.architecture}",
            format="zip",
            root_dir=sokketter_cli_zip_folder,
        )

        shutil.move(
            src=os.path.join(self.workspace, zip_name),
            dst=sokketter_cli_folder,
        )

        self.logger.info("CLI files packaged successfully.")

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

        if platform.system() == "Windows":
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
                base_name=f"sokketter-ui-{self.version}-{self.os_name}-{self.os_version}-{self.architecture}",
                format="zip",
                root_dir=sokketter_ui_zip_folder,
            )

            shutil.move(
                src=os.path.join(self.workspace, zip_name),
                dst=sokketter_ui_folder,
            )

        elif platform.system() == "Darwin":
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
                f"sokketter-ui-{self.version}-{self.os_name}-{self.os_version}-{self.architecture}.zip",
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
                f"sokketter-ui-{self.version}-{self.os_name}-{self.os_version}-{self.architecture}.dmg",
            )

            self.__create_dmg(app_path=app_filepath, output_path=dmg_filename)

        elif platform.system() == "Linux":
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
                    self.workspace, "sokketter-ui", "resources", "sokketter-ui-icon.png"
                ),
                sokketter_app_image_folder,
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

            linuxdeployqt_path = os.path.join(
                self.workspace, "linuxdeployqt-continuous-x86_64.AppImage"
            )

            packing_command = [
                linuxdeployqt_path,
                os.path.join(usr_bin_folder, "sokketter-ui"),
                "-appimage",
                f"-executable={os.path.join(usr_bin_folder, 'sokketter-ui')}",
                "-verbose=2",
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
                base_name=f"sokketter-ui-{self.version}-{self.os_name}-{self.os_version}-{self.architecture}",
                format="zip",
                root_dir=sokketter_ui_zip_folder,
            )

            shutil.move(
                src=os.path.join(self.workspace, zip_name),
                dst=sokketter_ui_folder,
            )

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

    args = parser.parse_args()

    if not args.stages:
        print("No stages specified. Use --help to see available stages.")
        sys.exit(1)

    Build(args.stages).run()
