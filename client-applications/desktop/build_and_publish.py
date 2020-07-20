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
from collections import defaultdict
from pathlib import Path
from typing import List, Dict, Optional
import requests

default_desktop_dir = Path(__file__).resolve().parent
default_protocol_dir = (  # This should match the default in retrieve_protocol_binary.py
    default_desktop_dir / ".." / "protocol-packages"
).resolve()

# Map between the responses from platf_mod.system() and Fractal's platform identifiers (without architecture size)
platform_map_no_size = {
    "darwin": "macOS",
    "linux": "Linux",
    "windows": "Windows",
}


default_channel_s3_buckets = {
    "testing": {
        "Windows-64bit": "fractal-applications-testing",
        "Linux-64bit": "fractal-linux-applications-testing",  # TODO as of 2020-07-18 this bucket does not exist!
        "macOS-64bit": "fractal-mac-application-testing",
    },
    "production": {
        "Windows-64bit": "fractal-applications-release",
        "Linux-64bit": "fractal-linux-applications-release",
        "macOS-64bit": "fractal-mac-applications-release",
    },
    "noupdates": defaultdict(
        lambda: "fractal: THIS IS AN INVALID S3 BUCKET - NO UPDATES WILL OCCUR"
    ),
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


def update_package_info(
    desktop_dir: Path,
    version: Optional[str] = None,
    publish_bucket: Optional[str] = None,
) -> None:
    # NOTE: the JSON does not sort keys because the order of the original package.json files should
    # be preserved since information is conveyed to the developer implicitly through order (such as importance of things)
    if publish_bucket:
        print(f"Setting update S3 bucket as '{publish_bucket}'")
        root_package_json = desktop_dir / "package.json"
        root_package = json.loads(root_package_json.read_text())
        root_package["build"]["publish"]["bucket"] = publish_bucket
        root_package_json.write_text(json.dumps(root_package, indent=4))

    if version:
        print(f"Setting version as '{version}'")
        app_package_json = desktop_dir / "app" / "package.json"
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
    # Anything codesigned must not have extra file attributes (it's unclear where these
    # attributes are coming from but they always appear to to occur, except for when the
    # protocol is built locally). This fix is from https://stackoverflow.com/a/39667628
    run_cmd(["xattr", "-c", str(client)])
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
            r.raise_for_status()
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


def package_via_yarn(desktop_dir):
    # FIXME left off here writing the build and publish material
    run_cmd([shutil.which("yarn")], cwd=desktop_dir)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    # General arguments
    parser.add_argument(
        "--platform",
        help="which platform to build",
        default=f"{platform_map_no_size[platf_mod.system().lower()]}-{'64' if sys.maxsize > 2**32 else '32'}bit",
    )
    parser.add_argument(
        "--protocol-version",
        help="which protocol package to use, either 'latest' or a substring of the desired package name (such as 'dev'); the file with the most recent modification time will be used, NOT the one with the highest version id (since they aren't comparable between refs/branches)",
        default="latest",
    )
    parser.add_argument(
        "--protocol-packages-dir",
        help="path to the directory where protocol packages are stored",
        default=str(default_protocol_dir),
    )
    parser.add_argument(
        "--src-dir",
        help="path to the client-apps desktop directory",
        default=str(default_desktop_dir),
    )
    # Auto update arguments
    parser.add_argument(
        "--update-channel",
        help="channel the auto update system should use (use {testing,production} to use the standard platform bucket for each channel, or 's3:BUCKET_NAME' for a custom location)",
        default="noupdates",
    )
    parser.add_argument(
        "--push-new-update",
        help="push the release to the auto update system (see --update-channel to define target)",
        action="store_true",
        default=False,
    )
    parser.add_argument(
        "--set-version",
        help="the version to set in the build (defaults to calculating a snapshot version from the current git state in the format 'SNAPSHOT-GIT_SHORT_SHA-{dirty,clean}' where 'dirty' is set if there are uncommitted local changes)",
    )
    parser.add_argument(
        "--override-version-check",
        help="by default, new versions will not be permitted to be published unless the follow the standard version format 'BRANCH-YYYYMMDD.#'; this flag can be used to override that if special circumstances permit",
        action="store_true",
        default=False,
    )
    # Misc. arguments
    parser.add_argument(
        "--cleanup",
        help="should any files created during this build process be removed (after a successful build, failed builds will leave files on disk for debugging)",
        action="store_true",
        default=False,
    )
    # macOS specific arguments
    parser.add_argument(
        "--macos-codesign-identity",
        help="string identifier for the codesign identity to sign the macOS client with",
        default="Fractal Computers, Inc.",
    )
    args = parser.parse_args()

    desktop_dir = Path(args.src_dir)
    protocol_packages_dir = Path(args.protocol_packages_dir)
    protocol_dir = (desktop_dir / "protocol-build").resolve()
    protocol_dir.mkdir(parents=True, exist_ok=True)

    cleanup_list = []
    protocol = select_protocol_binary(
        args.platform, args.protocol_version, protocol_packages_dir
    )

    # #####
    # Step: Protocol
    # #####
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

    # #####
    # Step: Platform Specific
    # #####
    system = platf_mod.system().lower()
    if "macos" in args.platform.lower() and system == "darwin":
        prep_unix(protocol_dir)
        prep_macos(
            protocol_dir=protocol_dir,
            desktop_dir=desktop_dir,
            codesign_identity=args.macos_codesign_identity,
        )
    elif "linux".lower() in args.platform.lower() and system == "linux":
        prep_unix(protocol_dir)
        prep_linux(protocol_dir)
    elif "windows" in args.platform.lower() and system == "windows":
        prep_windows(protocol_dir)
    else:
        raise Exception(
            f"This program is currently unable to cross compile to {args.platform} from your {sys.platform} system"
        )

    # #####
    # Step: Versioning & Auto Update Config
    # #####
    if ":" in args.update_channel:
        provider, bucket = args.update_channel.split(":")
        supported_providers = ["s3"]
        if provider not in supported_providers:
            raise Exception(
                "You're trying to use a custom auto update provider called '%s' but only {%s} are supported"
                % (provider, ",".join(supported_providers))
            )
        update_bucket = bucket
    else:
        update_bucket = default_channel_s3_buckets[args.update_channel][args.platform]  # type: ignore

    if args.set_version and not args.override_version_check:
        version_id_re = re.compile(r"(\S+)-(\d{8}).(\d+)")
        if not version_id_re.match(args.set_version):
            raise Exception(
                f"Invalid version ID: '{args.set_version}'. It must be in the form GITREF-YYYYMMDD.#, or --override-version-check must be passed."
            )

    version = args.set_version

    if not version:
        # shutil.which is installed in order to be cross-platform compatible with Windows (which
        # resolves commands differently since it doesn't look at the PATH env var when using
        # subprocess.run directly)
        git_cmd = shutil.which("git")
        if not git_cmd:
            raise Exception(
                "`git` must be installed in order to generate a version identifier"
            )
        # --porcelain designates an output that is meant to be parsed
        git_proc = subprocess.run(
            [git_cmd, "status", "--porcelain=v1"],
            stdout=subprocess.PIPE,
            cwd=desktop_dir,
        )
        dirty = git_proc.stdout.decode("utf-8").count("\n") > 0
        git_proc = subprocess.run(
            [git_cmd, "rev-parse", "--short", "HEAD"],
            stdout=subprocess.PIPE,
            cwd=desktop_dir,
        )
        short_sha = git_proc.stdout.decode("utf-8").strip()
        # NOTE: make sure this follows the format defined in the help message of args.version
        version = f"0.0.0-SNAPSHOT-{short_sha}-{'dirty' if dirty else 'clean'}"

    # NOTE: any updates to tracked files must occur after the `git status` calculation to determine
    # the `dirty`/`clean` status is checked otherwise the working directory will always be determined
    # to be `dirty`
    update_package_info(desktop_dir, version=version, publish_bucket=update_bucket)

    # #####
    # Step: Package & Publish
    # #####

    yarn_cmd = shutil.which("yarn")
    if not yarn_cmd:
        raise Exception(
            "`yarn` must be installed in order to build and package the application"
        )
    run_cmd([yarn_cmd], cwd=desktop_dir)  # Ensure that all dependencies are installed
    package_script = "package:publish" if args.push_new_update else "package"
    run_cmd([yarn_cmd, package_script], cwd=desktop_dir)

    # #####
    # Step: Cleanup
    # #####
    if args.cleanup:
        for trash in cleanup_list:
            print(f"Cleaning up '{trash}'")
            shutil.rmtree(trash, ignore_errors=True)
