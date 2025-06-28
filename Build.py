#!/usr/bin/env python

# --------------------------------------------------------------------------------------------------
#
# Imports.
#
# --------------------------------------------------------------------------------------------------
import logging
import os
import platform
import shutil
import subprocess

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
class Build:
    """
    Class to handle the build process.
    """

    def __init__(self) -> None:
        """
        Initialize the build class.
        """
        logging.basicConfig(
            level=logging.INFO,
            format="%(asctime)s - %(levelname)s - %(message)s",
        )
        self.logger = logging.getLogger(__name__)
        self.logger.info("Build class initialized.")

        self.workspace = os.environ.get("GITHUB_WORKSPACE", ".")
        self.logger.info("Workspace: %s", self.workspace)

        self.temp_build_output_dir = os.path.join(self.workspace, "build")
        self.logger.info("Temp build output directory: %s", self.temp_build_output_dir)

        self.temp_binary_output_dir = self.__construct_binary_output_dir(self.workspace)
        self.logger.info(
            "Temp binary output directory: %s", self.temp_binary_output_dir
        )

        self.results_output_dir = os.path.join(self.workspace, "results")
        self.logger.info("Results output directory: %s", self.results_output_dir)
        os.makedirs(self.temp_build_output_dir, exist_ok=True)

        self.cmake = self.__get_cmake()
        self.logger.info("CMake executable: %s", self.cmake)

        self.compiler = self.__get_cpp_compiler()
        self.logger.info("C++ compiler: %s", self.compiler)

        github_output_path = os.environ.get("GITHUB_OUTPUT")
        if github_output_path and os.path.exists(github_output_path):
            with open(file=github_output_path, mode="r", encoding="utf-8") as file:
                content = file.read()
                self.logger.info("Contents of GITHUB_OUTPUT file:\n%s", content)
        else:
            self.logger.info("GITHUB_OUTPUT file not found.")

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
        machine = platform.machine().lower()

        architecture = ""
        if machine in ["arm64", "aarch64"]:
            architecture = "arm64"
        else:
            architecture = "x86_64"

        if platform.system() == "Windows":
            return os.path.join(workspace, "bin", f"windows_{architecture}", "Release")
        elif platform.system() == "Linux":
            return os.path.join(workspace, "bin", f"linux_{architecture}", "Release")
        elif platform.system() == "Darwin":
            return os.path.join(workspace, "bin", f"macos_{architecture}", "Release")
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

    def __execute_command(self, cmake_command):
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
            ) as process:
                if process.stdout is not None:
                    for line in process.stdout:
                        self.logger.info(line.rstrip())

                process.wait()
                if process.returncode != 0:
                    raise subprocess.CalledProcessError(
                        process.returncode, cmake_command
                    )

        except subprocess.CalledProcessError as e:
            self.logger.error("Command failed with error:\n%s", e.stderr)
            raise

    def __configure(self) -> None:
        """
        Configure the project using CMake.
        """
        self.logger.info("Starting the CMake configuration.")

        if self.__running_in_github_actions():
            cmake_command = [
                self.cmake,
                "-B",
                self.temp_build_output_dir,
                f"-DCMAKE_CXX_COMPILER={self.compiler}",
                "-DIS_COMPILING_STATIC=true",
                "-DIS_COMPILING_SHARED=false",
                "-DCMAKE_PREFIX_PATH=$Qt6_DIR",
            ]
            self.__execute_command(cmake_command)
        else:
            raise EnvironmentError(
                "This script is intended to run in GitHub Actions environment only."
            )

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

    def __package_library(self) -> None:
        """
        Package the built library files.
        """
        self.logger.info("Starting the packaging of library files.")

        sokketter_lib_folder = os.path.join(self.results_output_dir, "libsokketter")
        os.makedirs(sokketter_lib_folder, exist_ok=True)

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
        os.makedirs(sokketter_cli_folder, exist_ok=True)

        if platform.system() == "Windows":
            shutil.copy(
                os.path.join(self.temp_binary_output_dir, "bin", "sokketter-cli.exe"),
                sokketter_cli_folder,
            )
        else:
            shutil.copy(
                os.path.join(self.temp_binary_output_dir, "bin", "sokketter-cli"),
                sokketter_cli_folder,
            )

        self.logger.info("CLI files packaged successfully.")

    def __package_ui(self) -> None:
        """
        Package the UI files.
        """
        self.logger.info("Starting the packaging of UI files.")

        sokketter_ui_folder = os.path.join(self.results_output_dir, "sokketter-ui")
        os.makedirs(sokketter_ui_folder, exist_ok=True)

        if platform.system() == "Windows":
            shutil.copy(
                os.path.join(self.temp_binary_output_dir, "bin", "sokketter-ui.exe"),
                sokketter_ui_folder,
            )
            packing_command = [
                "windeployqt",
                os.path.join(sokketter_ui_folder, "sokketter-ui.exe"),
                sokketter_ui_folder,
            ]
            self.__execute_command(packing_command)

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
        self.__configure()
        self.__build()
        self.__package()


# --------------------------------------------------------------------------------------------------
#
# Entry point.
#
# --------------------------------------------------------------------------------------------------
if __name__ == "__main__":
    Build().run()
