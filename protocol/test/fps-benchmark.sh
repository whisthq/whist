#!/bin/bash

# `fps-benchmark.sh` does the following:
#
# - Launches two containers with Chrome playing a looped video
# - Gets server.log and extracts the end-to-end time between frames
# - Prints the output (in ms, so lower is better)
# - Kills the containers
# 
# This should only be run on the dev machine

# Exit on subcommand errors
set -Eeuo pipefail

# Build the benchmark container
cd ~/whist/mandelboxes
./build_local_mandelbox_image.sh development/benchmark
WHIST_SKIP_EXEC_BASH=true ./run_local_mandelbox_image.sh development/benchmark
sleep 3s
WHIST_SKIP_EXEC_BASH=true ./run_local_mandelbox_image.sh development/benchmark
sleep 3s

# Get the time
sleep 10s
containers=$(docker container ls --quiet | head -n 1)
docker cp $containers:/usr/share/whist/server.log /tmp/server.log
cat /tmp/server.log | grep "Time between" | tr -s ' ' | cut -d' ' -f 17 | sed 's/.$//' | tail -n 5 | jq -s add/length

# Kill the containers
docker kill $(docker ps -q)
