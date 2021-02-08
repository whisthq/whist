#!/bin/bash

set -Eeuo pipefail

# Create temporary directory for build dependencies
mkdir -p "base/build-temp"
mkdir -p "base/build-temp/protocol"

# Copy protocol build into build-temp
cp ../protocol/server/build64/libsentry.so base/build-temp/protocol
cp ../protocol/server/build64/crashpad_handler base/build-temp/protocol
cp ../protocol/server/build64/FractalServer base/build-temp/protocol
