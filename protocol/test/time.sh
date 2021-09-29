#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

# Download the video
# This script expects to be run on the dev machine, so the paths are hard coded
cd ~/fractal/mandelboxes/development/benchmark
if [ ! -f bbb-1080p-75.mp4 ]; then
  curl https://fractal-test-assets.s3.amazonaws.com/bbb-1080p-75.mp4 -o bbb-1080p-75.mp4
fi

# Build the benchmark container
cd ~/fractal/mandelboxes
./build_local_mandelbox_image.sh development/benchmark
FRACTAL_SKIP_EXEC_BASH=true ./run_local_mandelbox_image.sh development/benchmark
sleep 3s
FRACTAL_SKIP_EXEC_BASH=true ./run_local_mandelbox_image.sh development/benchmark
sleep 3s

# Get the time
sleep 10s
containers=$(docker container ls --quiet | head -n 1)
docker cp $containers:/usr/share/fractal/server.log /tmp/server.log
cat /tmp/server.log | grep "Time between" | tr -s ' ' | cut -d' ' -f 17 | sed 's/.$//' | tail -n 5 | jq -s add/length

# Kill the containers
docker kill $(docker ps -q)
