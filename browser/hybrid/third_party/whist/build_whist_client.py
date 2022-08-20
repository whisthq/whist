#!/usr/bin/env python3
import argparse
import os
import subprocess
import platform
import paths
import shutil
from pathlib import Path

# This script is responsible for building the protocol client library for use by
# Chromium. It begins by generating a CMake build directory inside Chromium's GN
# build directory. Next, it builds the library into that directory. Finally, it
# copies the generated libraries to the root of Chromium's build directory.
#
# The script is invoked as an action() in BUILD.gn.

parser = argparse.ArgumentParser(description="Build the Whist Client Library.")
parser.add_argument(
    "--arch",
    type=str,
    required=(platform.system() == "Darwin"),
    help="The target architecture (x64 or arm64). Currently only used on macOS.",
)

COLORS = {
    "MAGENTA": "\033[95m",
    "GREEN": "\033[92m",
    "ENDC": "\033[0m",
}


def colored(text, color):
    return color + text + COLORS["ENDC"]


def command_to_string(command):
    return (
        colored(">>> ", COLORS["MAGENTA"])
        + colored(command[0], COLORS["GREEN"])
        + " "
        + " ".join(command[1:])
    )


def print_and_run(command):
    print(command_to_string(command))
    return subprocess.run(" ".join(command), shell=True, check=True)


args = parser.parse_args()

print("Configuring the Whist Client Library...")

arch_map = {
    "x64": "x86_64",
    "arm64": "arm64",
}

platform_specific_args = {
    "Darwin": ["-D", f"MACOS_TARGET_ARCHITECTURE={arch_map.get(args.arch, args.arch)}"],
    "Windows": ["-G", "Ninja"],
}

protocol_build_dir = os.path.join(os.getcwd(), "build-whist-client")
cmake_command = ["cmake"]
cmake_command += ["-S", paths.PROTOCOL_ROOT]
cmake_command += ["-B", protocol_build_dir]
cmake_command += ["-D", "CLIENT_SHARED_LIB=ON"]
cmake_command += platform_specific_args.get(platform.system(), [])
cmake_command += (
    ["-D", "USE_CCACHE=ON"] if os.path.basename(os.getenv("CC_WRAPPER")) == "ccache" else []
)

print_and_run(cmake_command)

print("Building the Whist Client Library...")
build_command = ["cmake", "--build", protocol_build_dir, "--target", "WhistClient"]

print_and_run(build_command)

print("Copying the Whist Client Library to the output directory...")
protocol_binaries_dir = os.path.join(protocol_build_dir, "client", "build64")
output_dir = os.getcwd()
platform_library_extensions = {
    "Darwin": "dylib",
    "Windows": "dll",
    "Linux": "so",
}
lib_ext = platform_library_extensions.get(platform.system(), "so")
libraries = Path(protocol_binaries_dir).glob(f"*.{lib_ext}")

for library in libraries:
    shutil.copy2(str(library.resolve()), output_dir)
