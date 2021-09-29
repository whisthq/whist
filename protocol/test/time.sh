#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

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
