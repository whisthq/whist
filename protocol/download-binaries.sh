#!/bin/bash

# Get shared libs
echo "Downloading Shared Libs"
curl -s "https://fractal-protocol-shared-libs.s3.amazonaws.com/shared-libs.tar.gz" | tar xzv

mkdir -p desktop/build64/Windows
mkdir -p server/build64/Windows
mkdir -p desktop/build64/Darwin

# in case we want to target Windows
cp share/64/Windows/* desktop/build64/Windows/
cp share/64/Windows/* server/build64

# copy OSX FFmpeg which is in lib/
cp lib/64/ffmpeg/Darwin/* desktop/build64/Darwin/

echo "Done"
