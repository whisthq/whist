#!/bin/bash

set -Eeuo pipefail

# TODO: We need to do something here which triggers a protocol `FractalServerMessage`
# that includes the uri in "$1"
echo "$1"
