#!/bin/bash
set -Eeuo pipefail

app_path=${1%/}
image=fractal/$app_path:current-build
mount=${2:-}

./run_container_image.sh $image $mount
