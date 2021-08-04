#!/bin/bash

# This script launches the FUSE filesystem for teleport mounted in the correct directory and
# running in the foreground, so that the PID of the FUSE filesystem is exactly what is returned
# by `getpid()` before the FUSE main function is called.

set -Eeuo pipefail

mkdir -p /home/fractal/.teleport/drag-drop/fuse
exec /usr/bin/fractal-teleport-fuse /home/fractal/.teleport/drag-drop/fuse -f > >(tee /home/fractal/teleport.log)
