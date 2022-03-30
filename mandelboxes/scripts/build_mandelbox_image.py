#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Usage: ./build_mandelbox_image.py [image_paths...] [-o] [--all]
# If -o is passed in, docker build will output to standard output
# If --all is passed in, all image paths in the repo will be built

import argparse
import re
import threading
import subprocess
import sys

# Argument parser
parser = argparse.ArgumentParser(description="Build Whist mandelbox image(s).")
parser.add_argument(
    "-q",
    "--quiet",
    action="store_true",
    help="This flag will make the docker builds silent",
)
parser.add_argument(
    "--all",
    action="store_true",
    help="This flag will have docker build every Dockerfile in the current directory",
)
parser.add_argument(
    "--mode",
    choices=["dev", "prod"],
    help=(
        "This flag controls when the protocol gets copied into the mandelbox. "
        "In dev mode, the protocol is copied at the end of each target "
        "mandelbox image. In prod mode, the protocol is only copied at the end of "
        "the base image and inherited by child images. If --all is passed in, this "
        "flag is ignored and prod mode is automatically set."
    ),
    required=True,
)
parser.add_argument("image_paths", nargs="*", help="List of image paths to build")
args = parser.parse_args()

# Input Variables
show_output = not args.quiet
# Remove trailing slashes
image_paths = [path.strip("/") for path in args.image_paths]
build_all = args.all
protocol_copy_mode = "prod" if build_all else args.mode
# Keep track of the initial targets for protocol copying purposes
target_image_paths = image_paths.copy()

# If --all is passed, generate image_paths procedurally
if build_all:
    with subprocess.Popen(
        "./scripts/find_mandelbox_dockerfiles_in_subtree.sh",
        shell=True,
        stdout=subprocess.PIPE,
    ) as files_process:
        image_paths = files_process.communicate()[0].decode("utf-8").strip().split(" ")
        print(
            "All files requested, will be building the following image paths: "
            + " ".join(image_paths)
        )

# Get dependency path from given image path
def get_dependency_from_image(img_path):  # returns dep_path
    # Open Dockerfile for the image path
    with open(img_path + "/Dockerfile.20", encoding="utf-8") as file:
        # Regex match the Dockerfile dependency, with a capture group on the dependency name
        regex = re.compile("^[ ]*FROM[ ]+whisthq/([^:]*):current-build")
        for line in file:
            result = regex.search(line)
            if result:
                # Grab the capture group
                dependency = result.group(1)

                # Return dependency and tag
                return dependency
    # Otherwise, return None for no dependency
    return None


# Map image paths to dependencies
dependencies = {}

# Get all dependencies for each requested image_path,
# and add all unrequested dependencies to the image_paths list.
# This ensures that every image_path is either a root node (dependency==None),
# or a child of another image_path
i = 0
while i < len(image_paths):
    image_path = image_paths[i]
    dep = get_dependency_from_image(image_path)
    dependencies[image_path] = dep
    if dep and dep not in image_paths:
        image_paths.append(dep)
    i += 1


def build_image_path(img_path, running_processes=None, ret=None, root_image=False):
    # Build image path
    print("Building " + img_path + "...")

    # Default is the build asset package without the protocol. By
    # choosing the correct build asset package and passing it as a
    # docker build argument, we can control which dockerfiles copy the
    # protocol!
    build_asset_package = "default"
    if protocol_copy_mode == "dev" and img_path in target_image_paths:
        build_asset_package = "protocol"
    if protocol_copy_mode == "prod" and root_image:
        build_asset_package = "protocol"

    command = [
        "docker",
        "build",
        "--rm",
        "--memory=4g",  # give Docker more memory to build the image
        "--memory-swap=-1",  # enable unlimited swap
        "--shm-size=4g",  # give Docker more memory to build the image
        "-f",
        f"{img_path}/Dockerfile.20",
        img_path,
        "-t",
        f"whisthq/{img_path}:current-build",
        "--build-arg",
        f"BuildAssetPackage={build_asset_package}",
    ]

    status = 0

    with open(f"{img_path}/build.log", "w", encoding="utf-8") as outfile:
        with subprocess.Popen(
            command,
            stdout=None if show_output else outfile,
            stderr=None if show_output else subprocess.STDOUT,
        ) as build_process:
            running_processes.append(build_process)
            build_process.wait()
            status = build_process.returncode
            if status is None:
                print(f"Error: Waited for {img_path} but got a return code of `None`")
                status = 1
            if status != 0:
                print(f"Failed to build {img_path}")
                print("Cancelling running builds...")
                for process in running_processes:
                    process.terminate()
                # We have to do this to get the return code in the parent
                # if this is running in a thread. We also return the same
                # value properly for the root image invocations to get a
                # returncode
                ret["status"] = status
                return ret["status"]

    # Notify successful build
    print(f"Built {img_path}")

    # Take all of the image_paths that depended on this img_path, and save them
    # as the next layer of image_paths to build
    next_layer = []
    for lhs, dependency in dependencies.items():
        if dependency and dependency == img_path:
            # Clear out dependencies that used to depend on img_path, as this
            # img_path has just finished building
            dependencies[lhs] = None
            # Save next layer img_path
            next_layer.append(lhs)

    # Build all of those next-layer image_paths asynchronously
    procs = []
    rets = []
    for next_image_path in next_layer:
        next_ret = {"status": None}
        proc = threading.Thread(
            name=f"docker build {next_image_path}",
            target=build_image_path,
            args=[next_image_path, running_processes, next_ret],
        )
        proc.start()
        procs.append(proc)
        rets.append(next_ret)
    # Wait for all the threads to finish executing before returning
    for proc, new_ret in zip(procs, rets):
        proc.join()
        if status == 0:
            if new_ret["status"] is not None:
                status = new_ret["status"]
            else:
                print("Error: Child thread exited with unset ret status!", new_ret)
                status = 1

    ret["status"] = status
    return ret["status"]


# Get all image_path's with no dependencies
if __name__ == "__main__":
    root_level_images = []
    for image_path, dependency_of_image in dependencies.items():
        if dependency_of_image is None:
            root_level_images.append(image_path)

    # Build all root level images
    for image_path in root_level_images:
        RET = build_image_path(
            image_path, running_processes=[], ret={"status": None}, root_image=True
        )
        if RET != 0:
            print("Failed to build some images")
            sys.exit(RET)

    print("All images built successfully!")
