#!/bin/bash

set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Working directory is fractal/container-images/
cd "$DIR"

mount=${1:-}

if [[ "$mount" == "mount" ]]; then
    shift # If the first parameter was "mount", shift that parameter away
    # Now rebuild the docker protocol to ensure that it is fresh
    ../protocol/build_protocol.sh
    # If no protocol exists, copy the current one, so that the base Dockerfile doesn't complain about it not existing
    # We intentionally don't update this, when mounting, so that the Docker image doesn't forcefully rebuild
    if [[ ! -d "base/build-temp/protocol" ]]; then
        ./helper-scripts/copy_protocol_build.sh
    fi
else
    # Otherwise, we copy_protocol_build.sh directly into the docker file

    # If copying the protocol build fails,
    # try compiling first and then copying again
    if ./helper-scripts/copy_protocol_build.sh ; then
        echo "A protocol build exists, though it is not guaranteed to be up-to-date."
    else
        echo "Attempting to copy an existing protocol build failed with the above error. Building ourselves a fresh copy..."
        ../protocol/build_protocol.sh
        ./helper-scripts/copy_protocol_build.sh
    fi
fi

# NVIDIA DRIVERS -- DOWNLOAD AND EXTRACT
mkdir -p "base/build-temp/nvidia-driver"
../ecs-host-setup/get-nvidia-driver-installer.sh && mv nvidia-driver-installer.run base/build-temp/nvidia-driver

python3 ./helper-scripts/build_container_image.py "$@"

echo "Cleaning up..."
rm -rf "base/build-temp"
