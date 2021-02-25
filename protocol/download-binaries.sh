#!/bin/bash

# This script downloads pre-built Fractal protocol libraries from AWS S3, on Unix

# Exit on errors and missing environment variables
set -Eeuo pipefail

echo "Downloading Protocol Libraries"

###############################
# Detect Operating System
###############################

unameOut="$(uname -s)"
case "${unameOut}" in
    CYGWIN*)      OS="Windows" ;;
    MINGW*)       OS="Windows" ;;
    *Microsoft*)  OS="Windows" ;;
    Linux*)
        if grep -qi Microsoft /proc/version; then
            OS="Windows+Linux"
        else
            OS="Linux"
        fi ;;
    Darwin*)      OS="Mac" ;;
    *)            echo "Unknown Machine: ${unameOut}" && exit 1 ;;
esac

###############################
# Create Directory Structure
###############################

mkdir -p desktop/build64/Windows
mkdir -p desktop/build64/Darwin
mkdir -p desktop/build64/Linux
mkdir -p server/build64
mkdir -p lib/64/SDL2

###############################
# Download Protocol Shared Libs
###############################

aws s3 cp --only-show-errors s3://fractal-protocol-shared-libs/shared-libs.tar.gz - | tar xz

# Copy Windows files
if [[ "$OS" =~ "Windows" ]]; then
    cp share/64/Windows/* desktop/build64/Windows/
    cp share/64/Windows/* server/build64
fi

# Copy macOS files
if [[ "$OS" == "Mac" ]]; then
    cp lib/64/ffmpeg/Darwin/* desktop/build64/Darwin/
fi

###############################
# Download SDL2 headers
###############################

mkdir -p include/SDL2
aws s3 cp --only-show-errors s3://fractal-protocol-shared-libs/fractal-sdl2-headers.tar.gz - | tar -xz -C include/SDL2

# Pull all SDL2 include files up a level and delete encapsulating folder
mv include/SDL2/include/* include/SDL2/
rm -rf include/SDL2/include

###############################
# Download SDL2 libraries
###############################

# Windows
if [[ "$OS" =~ "Windows" ]]; then
    mkdir -p lib/64/SDL2/Windows
    aws s3 cp --only-show-errors s3://fractal-protocol-shared-libs/fractal-windows-sdl2-static-lib.tar.gz - | tar xz -C lib/64/SDL2/Windows
fi

# macOS
if [[ "$OS" == "Mac" ]]; then
    mkdir -p lib/64/SDL2/Darwin
    aws s3 cp --only-show-errors s3://fractal-protocol-shared-libs/fractal-macos-sdl2-static-lib.tar.gz - | tar xz -C lib/64/SDL2/Darwin
fi

# Linux Ubuntu
if [[ "$OS" =~ "Linux" ]]; then
    mkdir -p lib/64/SDL2/Linux
    aws s3 cp --only-show-errors s3://fractal-protocol-shared-libs/fractal-linux-sdl2-static-lib.tar.gz - | tar xz -C lib/64/SDL2/Linux
fi

###############################
echo "Download Completed"
