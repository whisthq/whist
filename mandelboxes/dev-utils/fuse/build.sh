#!/bin/bash

set -Eeuo pipefail

docker build . --tag whist/mandelboxes/dev-utils/fuse:current-build
