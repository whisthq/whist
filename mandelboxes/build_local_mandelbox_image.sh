#!/bin/bash
set -Eeuo pipefail

# Get directory of bash shell and cd into folder
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR"

# Pull any changes to the Ubuntu 20.04 image
docker pull ubuntu:20.04

# Run with mounted fractal binary, rather than copying the binary into the mandelbox
./build_mandelbox_image.sh mount "$@"
