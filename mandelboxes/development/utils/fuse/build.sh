#!/bin/bash

set -Eeuo pipefail

docker build . --tag whist/mandelboxes/development/utils/fuse:current-build
