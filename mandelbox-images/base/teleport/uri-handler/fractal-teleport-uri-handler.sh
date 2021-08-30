#!/bin/bash

set -Eeuo pipefail

# Only write if there isn't a URL currently being handled
# Note that writes of size 4096 and under are atomic on
# Linux, so the only edge case is if there's a really long
# URL, in which case it's possible for it to be truncated.
if [[ ! -f /home/fractal/.teleport/handled-uri ]]; then
    echo -n "$1" > /home/fractal/.teleport/handled-uri
fi
