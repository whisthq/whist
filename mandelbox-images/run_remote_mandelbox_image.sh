#!/bin/bash

# This script runs a mandelbox image stored in GitHub Container Registry
# (GHCR). For it to work with the Fractal mandelboxes, this script needs to be
# run directly on a Fractal-enabled (see /host-setup) AWS EC2 instance, via
# SSH, without needing to build a mandelbox image beforehand.
# Arguments:
#    $1 - app name, including deploy environment (e.g. dev/browsers/chrome)
#    $2 - remote tag, i.e. git hash you wanna use
#    ...- any remaining arguments are passed to `run_mandelbox_image.sh`. Pass
#           in `--help` to see some helpful usage text.

set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# Working directory is fractal/mandelbox-images/
cd "$DIR"

# Parameters of the GHCR-stored mandelbox image to run
app_path=${1%/}
repo_name=fractal/$app_path
remote_tag=$2
ghcr_uri=ghcr.io
image=$ghcr_uri/$repo_name:$remote_tag

# Authenticate to GHCR
echo "$GH_PAT" | docker login --username "$GH_USERNAME" --password-stdin $ghcr_uri

# Download the image from GHCR
docker pull "$image"

# Run the image retrieved from GHCR, with the first two arguments (app_path and remote_tag) removed.
./helper_scripts/run_mandelbox_image.sh "$image" "${@:3}"
