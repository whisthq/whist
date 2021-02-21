#!/bin/bash

# This script runs a container image that exists locally on the machine where the script is run. For
# it to work with the Fractal containers, this script needs to be run directly on a Fractal-enabled (see /ecs-host-setup)
# AWS EC2 instance, via SSH, after the container image was locally built.

# Exit on errors and missing environment variables
set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# Working directory is fractal/container-images/
cd "$DIR"

# Parameters of the local container image to run
app_path=${1%/}
image=fractal/$app_path:current-build
mount=${2:-}

# Run the container image stored locally
./helper-scripts/run_container_image.sh "$image" "$mount"
