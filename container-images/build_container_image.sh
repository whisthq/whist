#!/bin/bash
set -Eeuo pipefail

local_tag=current-build
app_path=${1%/}

# build container with protocol inside it
docker build -f $app_path/Dockerfile.20 $app_path -t fractal/$app_path:$local_tag
