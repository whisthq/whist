#!/usr/bin/env python3
import argparse
import re
import json
import shutil
import sys
import platform as platf_mod
import subprocess
import stat
import traceback
from pathlib import Path
from typing import List, Dict
import requests

desktop_dir = Path(__file__).resolve().parent
repo_root = (desktop_dir / "..").resolve()
default_protocol_dir = (
    repo_root / "protocol-packages"
)  # This should match the default in retrieve_protocol_binary.py

# Map between the responses from platf_mod.system() and Fractal's platform identifiers
platform_map = {
    "darwin": "macOS",
    "linux": "Linux",
    "windows": "Windows",
}


def select_protocol_binary(platform: str, protocol_id: str, protocol_dir: Path) -> Path:
    valid_protocols = protocol_dir.glob(f"*{platform}*.*")
    mtime_sorted_protocols = sorted(
        map(lambda p: (p, p.stat().st_mtime), valid_protocols), key=lambda b: b[1],
    )
    if len(mtime_sorted_protocols) < 1:
        raise Exception(
            f"Unable to find any protocol packages for {platform} in {str(protocol_dir)}. Maybe you need to run `retrieve_protocol_packages.py` to download one for this platform."
        )

    if protocol_id == "latest":
        return mtime_sorted_protocols[-1][0]

    for package, mtime in mtime_sorted_protocols:
        if protocol_id in package.name:
            return package

    raise Exception(f"Unable to find a protocol package containing '{protocol_id}'")


def update_package_versions(src_dir: Path, version: str, publish_bucket: str) -> None:
    root_package_json = src_dir / "package.json"
    root_package = json.loads(root_package_json.read_text())
    root_package["build"]["publish"]["bucket"] = publish_bucket
    root_package_json.write_text(json.dumps(root_package, indent=4))

    app_package_json = src_dir / "app" / "package.json"
    app_package = json.loads(app_package_json.read_text())
    app_package["version"] = version
    app_package_json.write_text(json.dumps(app_package, indent=4))


def run_cmd(cmd: List[str], **kwargs):
    print("run: " + " ".join(cmd))
    return subprocess.run(cmd, check=True, **kwargs)


def prep_unix(protocol_dir: Path) -> None:  # Shared by Linux and macOS
    (protocol_dir / "sshkey").chmod(0o600)


def prep_macos(desktop_dir: Path, protocol_dir: Path, codesign_identity: str) -> None:
    client = protocol_dir / "FractalClient"
    # Add logo to the FractalClient executable
    # TODO The "sips" command appears to do nothing. Test that icons are successfully
    # added without it and then feel free to remove it.
    run_cmd( 
        ["sips", "-i", str(desktop_dir / "build" / "icon.png")]
    )  # take an image and make the image its own icon
    src_icon = desktop_dir / "build" / "icon.png"
    tmp_icon = protocol_dir / "tmpicns.rsrc"
    with tmp_icon.open("w") as f:
        run_cmd(  # extract the icon to its own resource file
            ["DeRez", "-only", "icns", str(src_icon)], stdout=f
        )
    run_cmd(  # append this resource to the file you want to icon-ize
        ["Rez", "-append", str(tmp_icon), "-o", str(client)]
    )
    run_cmd(["SetFile", "-a", "C", str(client)])  # use the resource to set the icon
    tmp_icon.unlink()
    # codesign the FractalClient executable
    run_cmd(["codesign", "-s", codesign_identity, str(client)])


def prep_linux(protocol_dir: Path) -> None:
    client = protocol_dir / "FractalClient"
    client.chmod(
        client.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH
    )  # this long chain is equivalent to "chmod +x"
    unison = protocol_dir / "linux_unison"
    unison.chmod(
        unison.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH
    )  # this long chain is equivalent to "chmod +x"


def prep_windows(protocol_dir: Path) -> None:
    rcedit_path = desktop_dir / "rcedit-x64.exe"
    cleanup_list.append(rcedit_path)
    if not rcedit_path.exists():
        rcedit_version = "v1.1.1"
        print(f"Downloading rcedit-x64.exe {rcedit_version}")
        with requests.get(
            f"https://github.com/electron/rcedit/releases/download/{rcedit_version}/rcedit-x64.exe",
            stream=True,
        ) as r:
            with open(rcedit_path) as f:
                shutil.copyfileobj(r.raw, f)
    rcedit_cmd = [
        str(rcedit_path),
        str(protocol_dir / "FractalClient.exe"),
        "--set-icon",
        "build\\icon.ico",
    ]
    print("Updating FractalClient icon using `%s`" % " ".join(rcedit_cmd))
    subprocess.run(rcedit_cmd, check=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        dest="platforms",
        metavar="platform[:protocol_file|latest]",
        nargs="?",
        help="which platforms to build with which protocol package; if 'latest' is specified for a protocol package than the file with the most recent modification time will be used NOT with the highest version id (since they aren't comparable between refs)",
        default=f"{platform_map[platf_mod.system().lower()]}-{'64' if sys.maxsize > 2**32 else '32'}bit:latest",
    )
    parser.add_argument(
        "--protocol-packages-dir",
        help="path to the directory where protocol packages are stored",
        default=str(default_protocol_dir),
    )
    parser.add_argument(
        "--src-dir",
        help="path to the client-apps desktop directory",
        default=str(desktop_dir),
    )
    parser.add_argument(
        "--macos-codesign-identity",
        help="string identifier for the codesign identity to sign the macOS client with",
        default="Fractal Computers, Inc.",
    )
    parser.add_argument(
        "--cleanup",
        help="should any files created during this build process be removed (after a successful build, failed builds will leave files on disk for debugging)",
        action="store_true",
        default=False,
    )
    parser.add_argument("--version", help="", default="")
    args = parser.parse_args()

    protocol_packages_dir = Path(args.protocol_packages_dir)
    protocol_dir = Path(args.src_dir) / "protocol-build" / "desktop"

    # This is structured as a loop as though it could compile for multiple architectures, however,
    # cross-compiling is currently unsupported and therefore no caller will likely leverage this
    # feature.
    platforms = args.platforms.split(",")
    for platform_pair in platforms:
        if ":" not in platform_pair:
            platform_pair += ":latest"
        try:
            cleanup_list = []
            platform, protocol_id = platform_pair.split(":")
            protocol = select_protocol_binary(
                platform, protocol_id, protocol_packages_dir
            )

            # NOTE: the protocol-build directory is re-used for each platform
            shutil.rmtree(protocol_dir, ignore_errors=True)
            print(f"Unpacking '{protocol}' to '{protocol_dir}'")
            shutil.unpack_archive(protocol, protocol_dir)
            cleanup_list.append(protocol_dir)
            # Depending on how the archive was created it may place the files (like "FractalClient") directly
            # in the root of the destination (ie. protocol_dir), or it might create an intermediate folder
            # in the destination that the files are then placed in to. This will use a simple heuristic
            # to check for and remove the intermediate folder if it is present.
            heuristic_protocol_dir = list(protocol_dir.glob("*"))
            if len(heuristic_protocol_dir) == 1 and heuristic_protocol_dir[0].is_dir():
                print(
                    f"Removing intermediate directory '{heuristic_protocol_dir[0].name}' created when unpacking"
                )
                for item in heuristic_protocol_dir[0].iterdir():
                    shutil.move(str(item), str(protocol_dir))
                # Use rmdir instead of shutil.rmtree because it won't delete an empty directory which acts as
                # a check to ensure the move operations succeeded
                heuristic_protocol_dir[0].rmdir()
            # Sanity check that FractalClient is in the unpacked directory so that an early
            # failure can be triggered if the unpacking failed or the release was zipped up
            # in an unexpected manner
            if len(list(protocol_dir.glob("FractalClient*"))) != 1:
                raise Exception(
                    f"The unpacked protocol does not match the expected format in '{protocol_dir}'"
                )

            system = platf_mod.system().lower()
            if "macos" in platform.lower() and system == "darwin":
                prep_unix(protocol_dir)
                prep_macos(
                    protocol_dir=protocol_dir,
                    desktop_dir=desktop_dir,
                    codesign_identity=args.macos_codesign_identity,
                )
            elif "linux".lower() in platform.lower() and system == "linux":
                prep_unix(protocol_dir)
                prep_linux(protocol_dir)
            elif "windows" in platform.lower() and system == "windows":
                prep_windows(protocol_dir)
            else:
                raise Exception(
                    f"This program is currently unable to cross compile to {platform} from your {sys.platform} system"
                )

            if args.cleanup:
                for trash in cleanup_list:
                    print(f"Cleaning up '{trash}'")
                    shutil.rmtree(trash, ignore_errors=True)

        except Exception:
            print(traceback.format_exc())
