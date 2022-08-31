#!/bin/bash

set -Eeuo pipefail

docker build . --quiet --tag whist/mandelboxes/development/utils/fuse:current-build
