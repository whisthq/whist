#!/bin/bash

# This script runs a container image that exists locally on the machine where the script is run. For 
# it to work with the Fractal containers, this script needs to be run directly on a Fractal-enabled (see /ecs-host-setup)
# AWS EC2 instance, via SSH, after the container image was locally built.

set -Eeuo pipefail

# Parameters of the local container image to run
app_path=${1%/}
image=fractal/$app_path:current-build
mount=${2:-}

# Run the container image stored locally
./run_container_image.sh $image $mount
