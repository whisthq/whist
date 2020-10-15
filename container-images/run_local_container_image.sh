#!/bin/bash
set -Eeuo pipefail

image=fractal/$1:current-build
mount=${2:-}

./run_container_image.sh $image $mount
