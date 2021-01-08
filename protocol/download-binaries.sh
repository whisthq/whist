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

# Get SDL2 includes
mkdir include/SDL2
curl -s "https://fractal-protocol-shared-libs.s3.amazonaws.com/fractal-sdl2-headers.tar.gz" | tar -xzv -C include/SDL2

# Pull all SDL2 include files up a level and delete encapsulating folder
mv include/SDL2/include/* include/SDL2/
rm -rf include/SDL2/include

# Get SDL2 shared libraries
mkdir lib/64/SDL2
mkdir lib/64/SDL2/Darwin
curl -s "https://fractal-protocol-shared-libs.s3.amazonaws.com/fractal-macos-sdl2-static-lib.tar.gz" | tar xzv -C lib/64/SDL2/Darwin
mkdir lib/64/SDL2/Linux
curl -s "https://fractal-protocol-shared-libs.s3.amazonaws.com/fractal-linux-sdl2-static-lib.tar.gz" | tar xzv -C lib/64/SDL2/Linux
mkdir lib/64/SDL2/Windows
curl -s "https://fractal-protocol-shared-libs.s3.amazonaws.com/fractal-windows-sdl2-static-lib.tar.gz" | tar xzv -C lib/64/SDL2/Windows

echo "Done"
