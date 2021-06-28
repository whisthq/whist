#!/bin/bash

# This script runs a container image that exists locally on the machine where
# the script is run. For it to work with the Fractal containers, this script
# needs to be run directly on a Fractal-enabled (see /ecs-host-setup) AWS EC2
# instance, via SSH, after the container image was locally built.

set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# Working directory is fractal/mandelbox-images/
cd "$DIR"

# We add the `fractal/` prefix and `:current-build` tag to the image name, then
# call `run_mandelbox_image.sh` with that image name and any other arguments
# provided.
app_path=${1%/}
image=fractal/$app_path:current-build
./helper_scripts/run_mandelbox_image.sh "$image" --update-protocol=True "${@:2}"
