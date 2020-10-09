#!/bin/bash
set -Eeuo pipefail

local_tag=current-build

# build container with protocol inside it
docker build -f $1/Dockerfile.20 $1 -t fractal/$1:$local_tag
