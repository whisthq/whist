#!/bin/bash

# Get shared libs
echo "Downloading Shared Libs"
aws s3 cp s3://fractal-protocol-shared-libs/shared-libs.tar.gz - | tar xz

mkdir -p desktop/build64/Windows
mkdir -p server/build64/Windows
mkdir -p desktop/build64/Darwin

# in case we want to target Windows
cp share/64/Windows/* desktop/build64/Windows/
cp share/64/Windows/* server/build64

# copy OSX FFmpeg which is in lib/
cp lib/64/ffmpeg/Darwin/* desktop/build64/Darwin/

# Get SDL2 includes
mkdir include/SDL2
aws s3 cp s3://fractal-protocol-shared-libs/fractal-sdl2-headers.tar.gz - | tar -xz -C include/SDL2

# Pull all SDL2 include files up a level and delete encapsulating folder
mv include/SDL2/include/* include/SDL2/
rm -rf include/SDL2/include

# Get SDL2 shared libraries
mkdir lib/64/SDL2
mkdir lib/64/SDL2/Darwin
aws s3 cp s3://fractal-protocol-shared-libs/fractal-macos-sdl2-static-lib.tar.gz - | tar xz -C lib/64/SDL2/Darwin
mkdir lib/64/SDL2/Linux
aws s3 cp s3://fractal-protocol-shared-libs/fractal-linux-sdl2-static-lib.tar.gz - | tar xz -C lib/64/SDL2/Linux
mkdir lib/64/SDL2/Windows
aws s3 cp s3://fractal-protocol-shared-libs/fractal-windows-sdl2-static-lib.tar.gz - | tar xz -C lib/64/SDL2/Windows

echo "Completed"
