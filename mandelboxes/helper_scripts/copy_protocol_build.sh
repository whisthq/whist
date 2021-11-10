#!/bin/bash

set -Eeuo pipefail

# Retrieve source directory of this script
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR/.."

destdir=${1:-.build-temp}

# Create temporary directory for build dependencies
mkdir -p "$destdir/protocol"

# Copy protocol build into build-temp
SOURCE_DIR="../protocol"
BUILD_DIR=""

if [[ "${2}" == "FractalServer" ]]; then
	BUILD_DIR="../protocol/build-docker/server/build64"
else
	BUILD_DIR="../protocol/build-docker/client/build64"
fi

# Verify that the FractalServer/FractalClient is there
cp -r "$BUILD_DIR"/${2} "$destdir/protocol"

# Copy all of the accompanying files
cp -r "$BUILD_DIR"/* "$destdir/protocol"

# Copy dependencies script
cp "$SOURCE_DIR/setup-linux-build-environment.sh" "$destdir"
