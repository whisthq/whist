#!/bin/bash

# This script installs dev dependencies (at this time all python related) that are ONLY relevant for Fractal engineer's dev instances

set -Eeuo pipefail

sudo apt-get install -y python3-pip
cd ~/fractal
find ./container-images -name 'requirements.txt' | sed 's/^/-r /g' | xargs sudo pip3 install
cd ~/fractal/ecs-host-setup