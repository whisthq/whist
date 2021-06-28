#!/bin/bash

set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Set working dir to fractal/mandelbox-images/helper_scripts
cd "$DIR"

# Unfortunately, python's subprocess module doesn't give us an easy way to run
# interactive programs (i.e. with standard in/out), so we run the actual bash
# command in the container via this script instead of in the python file. We
# need to set up some fancy indirection to store the last line of the script's
# output (which should be the docker container's ID) as $docker_id.
# see https://stackoverflow.com/questions/39615142/bash-get-last-line-from-a-variable
output=$(sudo python3 run_container_image.py "$@" | tee /dev/tty)
docker_id="${output##*$'\n'}"

# Start bash in the container.
docker exec -it "$docker_id" /bin/bash || true

# Kill the container once we're done
docker kill "$docker_id" > /dev/null
docker rm "$docker_id" > /dev/null
