#!/bin/bash

# This script runs a mandelbox image that exists locally on the machine where
# the script is run. For it to work with the Whist mandelboxes, this script
# needs to be run directly on a Whist-enabled (see /host-setup) AWS EC2
# instance, via SSH, after the mandelbox image was locally built.

set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Working directory is whist/mandelboxes/
cd "$DIR"

# We add the `whisthq/` prefix and `:current-build` tag to the image name, then
# call `run_mandelbox_image.sh` with that image name and any other arguments
# provided.
app_path=${1%/}
image=whisthq/$app_path:current-build
./scripts/run_mandelbox_image.sh "$image" "${@:2}"
