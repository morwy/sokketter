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
        if shutil.which(cmake):
            return cmake

        self.logger.error("CMake executable not found at default location: %s", cmake)

        if platform.system() == "Windows":
            cmake = "C:\\Qt\\Tools\\CMake_64\\bin\\cmake.exe"
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
        elif platform.system() == "Linux":
            return os.path.join(
                workspace, "bin", f"linux_{self.architecture}", "Release"
            )
        elif platform.system() == "Darwin":
            return os.path.join(
                workspace, "bin", f"macos_{self.architecture}", "Release"
            )
        else:
            self.logger.error("Unsupported platform: %s", platform.system())
            raise EnvironmentError("Unsupported platform")

    def __running_in_github_actions(self) -> bool:
        return os.getenv("GITHUB_ACTIONS") == "true"

    def __get_vcvarsall_path(self) -> str:
        """
        Get the path to the vcvarsall.bat file for Visual Studio.
        """
        root_paths = [
            r"C:\Program Files\Microsoft Visual Studio\2022\Community",
            r"C:\Program Files\Microsoft Visual Studio\2022\Professional",
            r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise",
            r"C:\Program Files\Microsoft Visual Studio\2019\Community",
            r"C:\Program Files\Microsoft Visual Studio\2019\Professional",
            r"C:\Program Files\Microsoft Visual Studio\2019\Enterprise",
            r"C:\Program Files\Microsoft Visual Studio\2017\Community",
            r"C:\Program Files\Microsoft Visual Studio\2017\Professional",
            r"C:\Program Files\Microsoft Visual Studio\2017\Enterprise",
            r"C:\Program Files\Microsoft Visual Studio\2022\BuildTools",
            r"C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools",
            r"C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools",
        ]

        for root_path in root_paths:
            internal_path = os.path.join("VC", "Auxiliary", "Build", "vcvarsall.bat")
            vcvarsall_path = os.path.join(root_path, internal_path)

            if os.path.exists(vcvarsall_path):
                self.logger.info("vcvarsall.bat was found at: %s", vcvarsall_path)
                return vcvarsall_path

        self.logger.error("vcvarsall.bat not found!")
        raise EnvironmentError("vcvarsall.bat not found!")

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

    def __execute_command(self, cmake_command, cwd: str | None = None):
        try:
            self.logger.info(
                "Executing command: %s",
                (
                    " ".join(cmake_command)
                    if isinstance(cmake_command, list)
                    else cmake_command
                ),
            )

            with subprocess.Popen(
                cmake_command,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                cwd=cwd,
            ) as process:
                if process.stdout is not None:
                    for line in process.stdout:
                        self.logger.info(line.rstrip())

                process.wait()
                if process.returncode != 0:
                    raise subprocess.CalledProcessError(
                        process.returncode, cmake_command
                    )

        except FileNotFoundError as e:
            self.logger.error("Command executable not found: %s", e)
            raise

        except subprocess.CalledProcessError as e:
            self.logger.error("Command failed with error:\n%s", e.stderr)
            raise

    def __configure(self) -> None:
        """
        Configure the project using CMake.
        """
        self.logger.info("Starting the CMake configuration.")

        cmake_command = [
            self.cmake,
            "-B",
            self.temp_build_output_dir,
            f"-DCMAKE_CXX_COMPILER={self.compiler}",
            "-DIS_COMPILING_STATIC=true",
            "-DIS_COMPILING_SHARED=false",
            "-DCMAKE_PREFIX_PATH=$Qt6_DIR",
        ]

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
                set resizable of container window to false

                tell icon view options of container window
                    set icon size to 128
                    set arrangement to not arranged
                    set text size to 12
                    set shows item info to false
                    set shows icon preview to true
                end tell

                -- Centered horizontally, roughly around the middle of the window
                set position of item "sokketter-ui.app" to {{180, 200}}
                set position of item "Applications" to {{460, 200}}

                -- 640 x 400 window
                set bounds of container window to {{100, 100, 740, 600}}

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

    def __create_dmg(self, app_path: str, output_path: str):
        app = pathlib.Path(app_path).resolve()
        output = pathlib.Path(output_path).resolve()

        with tempfile.TemporaryDirectory() as tmp:
            staging = pathlib.Path(tmp) / "sokketter-ui"
            staging.mkdir()

            # Copy application
            shutil.copytree(app, staging / app.name)

            # Applications symlink
            (staging / "Applications").symlink_to("/Applications")

            # Create read/write DMG first
            rw_dmg = pathlib.Path(tmp) / "sokketter-rw.dmg"

            subprocess.run(
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
                check=True,
            )

            # Mount it
            result = subprocess.run(
                [
                    "hdiutil",
                    "attach",
                    "-readwrite",
                    "-noverify",
                    "-noautoopen",
                    str(rw_dmg),
                ],
                check=True,
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

            try:
                self.__configure_finder(mount_point)

                # Unmount
                subprocess.run(["hdiutil", "detach", str(mount_point)], check=True)

                # Compress to final DMG
                subprocess.run(
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
                    check=True,
                )

            finally:
                # Best effort cleanup if something failed
                subprocess.run(
                    ["hdiutil", "detach", str(mount_point)],
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL,
                )

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
