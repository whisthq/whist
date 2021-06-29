# Usage: ./build_mandelbox_image.py [image_paths...] [-o] [--all]
# If -o is passed in, docker build will output to standard output
# If --all is passed in, all image paths in the repo will be built

import argparse
import re
import threading
import subprocess
import sys

# Argument parser
parser = argparse.ArgumentParser(description="Build Fractal mandelbox image(s).")
parser.add_argument(
    "-o",
    "--show-output",
    action="store_true",
    help="This flag will have docker build output print to stdout",
)
parser.add_argument(
    "--all",
    action="store_true",
    help="This flag will have docker build every Dockerfile in the current directory",
)
parser.add_argument("image_paths", nargs="*", help="List of image paths to build")
args = parser.parse_args()

# Input Variables
show_output = args.show_output
# Remove trailing slashes
image_paths = [path.strip("/") for path in args.image_paths]
build_all = args.all

# If --all is passed, generate image_paths procedurally
if build_all:
    with subprocess.Popen(
        "./helper_scripts/find_images_in_git_repo.sh",
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
    with open(img_path + "/Dockerfile.20") as file:
        # Regex match the Dockerfile dependency, with a capture group on the dependency name
        regex = re.compile("^[ ]*FROM[ ]+fractal/([^:]*):current-build")
        for line in file:
            result = regex.search(line)
            if result:
                # Grab the capture group
                dependency = result.group(1)

                # Return dependency and tag
                return dependency
    # Otherwise, return None for no dependency
    return None


# Map image_path's to dependencies
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


def build_image_path(img_path):
    # Build image path
    print("Building " + img_path + "...")
    command = (
        "docker build -f "
        + img_path
        + "/Dockerfile.20 "
        + img_path
        + " -t fractal/"
        + img_path
        + ":current-build"
    )
    if not show_output:
        command += " >> .build_output 2>&1"

    with subprocess.Popen(command, shell=True) as build_process:
        build_process.wait()
        if build_process.returncode != 0:
            # If _any_ build fails, we exit with return code 1
            print("Build of " + img_path + " failed, terminating")
            sys.exit(1)

    # Notify successful build
    print("Built " + img_path + "!")

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
    for next_image_path in next_layer:
        proc = threading.Thread(
            name="Builder of " + next_image_path,
            target=build_image_path,
            args=[next_image_path],
        )
        proc.start()
        procs.append(proc)
    # Wait for all the threads to finish executing before returning
    for proc in procs:
        proc.join()


# Get all image_path's with no dependencies
root_level_images = []
for image_path, dependency_of_image in dependencies.items():
    if dependency_of_image is None:
        root_level_images.append(image_path)

# Build all root_level_images
for image_path in root_level_images:
    build_image_path(image_path)

print("All images have been built successfully!")
