#!/bin/bash

set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Working directory is fractal/container-images/
cd "$DIR"

# If copying the protocol build fails,
# try compiling first and then copying again
if ./helper-scripts/copy_protocol_build.sh ; then
    echo "A protocol build exists, though it is not guaranteed to be up-to-date."
else
    echo "Attempting to copy an existing protocol build failed with the above error. Building ourselves a fresh copy..."
    ../protocol/build_protocol.sh
    ./helper-scripts/copy_protocol_build.sh
fi

# NVIDIA DRIVERS -- DOWNLOAD AND EXTRACT
mkdir -p "base/build-temp/nvidia-driver"
../ecs-host-setup/get-nvidia-driver-installer.sh && mv nvidia-driver-installer.run base/build-temp/nvidia-driver

python3 ./helper-scripts/build_container_image.py $@

echo "Cleaning up..."
rm -rf "base/build-temp"
