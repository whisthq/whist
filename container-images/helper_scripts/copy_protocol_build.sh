#!/bin/bash

set -Eeuo pipefail

# Retrieve source directory of this script
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR/.."

# Create temporary directory for build dependencies
mkdir -p "base/build-temp/protocol"

# Copy protocol build into build-temp
SOURCE_DIR="../protocol"
BUILD_DIR="../protocol/build-docker/server/build64"

# Verify that the FractalServer is there
cp "$BUILD_DIR"/FractalServer base/build-temp/protocol

# Copy all of the accompanying files
cp "$BUILD_DIR"/* base/build-temp/protocol

# Copy dependencies script
cp "$SOURCE_DIR/setup-linux-build-environment.sh" base/build-temp
