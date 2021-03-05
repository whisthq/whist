#!/bin/bash

# This script downloads pre-built Fractal protocol libraries from AWS S3, on Unix

# Exit on errors and missing environment variables
set -Eeuo pipefail

echo "Downloading Protocol Libraries"

touch .libcache
function has_updated {
    # Memoize AWS_LIST, which includes filenames and timestamps
    if [[ -z "${AWS_LIST+x}" ]]; then
        export AWS_LIST="$(aws s3 ls s3://fractal-protocol-shared-libs)"
    fi
    TIMESTAMP_LINE=$(echo "$AWS_LIST" | grep " $1" | awk '{print $4, $1, $2}')
    if grep "$TIMESTAMP_LINE" .libcache &>/dev/null; then
        return 1 # Return false since the lib hasn't updated
    else
        # Delete old timestamp line for that lib, if it exists
        sed -i "/^$1 /d" .libcache
        # Append new timestamp line to .libcache
        echo "$TIMESTAMP_LINE" >> .libcache
        # Print found library
        echo "$TIMESTAMP_LINE" | awk '{print $1}'
        # Return true, since the lib has indeed updated
        return 0
    fi
}

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

LIB="shared-libs.tar.gz"
if has_updated "$LIB"; then
    mkdir -p shared-libs
    aws s3 cp --only-show-errors s3://fractal-protocol-shared-libs/$LIB - | tar -xz -C shared-libs

    # Copy Windows files
    if [[ "$OS" =~ "Windows" ]]; then
        cp shared-libs/share/64/Windows/* desktop/build64/Windows/
        cp shared-libs/share/64/Windows/* server/build64
    fi

    rm -r shared-libs
fi

###############################
# Copy shared libs
###############################

# Copy macOS files
if [[ "$OS" == "Mac" ]]; then
    cp lib/64/ffmpeg/Darwin/* desktop/build64/Darwin/
fi

###############################
# Download SDL2 headers
###############################

mkdir -p include/SDL2
LIB="fractal-sdl2-headers.tar.gz"
if has_updated "$LIB"; then
    aws s3 cp --only-show-errors s3://fractal-protocol-shared-libs/$LIB - | tar -xz -C include/SDL2

    # Pull all SDL2 include files up a level and delete encapsulating folder
    mv include/SDL2/include/* include/SDL2/
    rm -rf include/SDL2/include
fi

###############################
# Download SDL2 libraries
###############################

# Windows
if [[ "$OS" =~ "Windows" ]]; then
    mkdir -p lib/64/SDL2/Windows
    LIB="fractal-windows-sdl2-static-lib.tar.gz"
    if has_updated "$LIB"; then
        aws s3 cp --only-show-errors s3://fractal-protocol-shared-libs/fractal-windows-sdl2-static-lib.tar.gz - | tar -xz -C lib/64/SDL2/Windows
    fi
fi

# macOS
if [[ "$OS" == "Mac" ]]; then
    mkdir -p lib/64/SDL2/Darwin
    LIB="fractal-macos-sdl2-static-lib.tar.gz"
    if has_updated "$LIB"; then
        aws s3 cp --only-show-errors s3://fractal-protocol-shared-libs/fractal-macos-sdl2-static-lib.tar.gz - | tar -xz -C lib/64/SDL2/Darwin
    fi
fi

# Linux Ubuntu
if [[ "$OS" =~ "Linux" ]]; then
    mkdir -p lib/64/SDL2/Linux
    LIB="fractal-linux-sdl2-static-lib.tar.gz"
    if has_updated "$LIB"; then
        aws s3 cp --only-show-errors s3://fractal-protocol-shared-libs/fractal-linux-sdl2-static-lib.tar.gz - | tar -xz -C lib/64/SDL2/Linux
    fi
fi

###############################
echo "Download Completed"
