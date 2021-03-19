#!/bin/bash

# This script downloads pre-built Fractal protocol libraries from AWS S3, on Unix

# Exit on errors and missing environment variables
set -Eeuo pipefail

###############################
# Detect Parameters
###############################

OS="$1"
CLIENT_DIR="$2"
SERVER_DIR="$3"
SOURCE_DIR="$4"
CACHE_DIR="$5"
mkdir -p "$CACHE_DIR"

echo "Downloading Protocol Libraries"

CACHE_FILE="$CACHE_DIR/.libcache"
touch "$CACHE_FILE"
function has_updated {
    # Memoize AWS_LIST, which includes filenames and timestamps
    if [[ -z "${AWS_LIST+x}" ]]; then
        export AWS_LIST="$(aws s3 ls s3://fractal-protocol-shared-libs)"
    fi
    TIMESTAMP_LINE=$(echo "$AWS_LIST" | grep " $1" | awk '{print $4, $1, $2}')
    if grep "$TIMESTAMP_LINE" "$CACHE_FILE" &>/dev/null; then
        return 1 # Return false since the lib hasn't updated
    else
        # Delete old timestamp line for that lib, if it exists
        NEW_LIBCACHE=$(sed "/^$1 /d" "$CACHE_FILE")
        echo "$NEW_LIBCACHE" > "$CACHE_FILE"
        # Append new timestamp line to libcache
        echo "$TIMESTAMP_LINE" >> "$CACHE_FILE"
        # Print found library
        echo "$TIMESTAMP_LINE" | awk '{print $1}'
        # Return true, since the lib has indeed updated
        return 0
    fi
}

###############################
# Download Protocol Shared Libs
###############################

LIB="shared-libs.tar.gz"
if has_updated "$LIB"; then
    SHARED_LIBS_DIR="$CACHE_DIR/shared-libs"
    mkdir -p "$SHARED_LIBS_DIR"
    aws s3 cp --only-show-errors "s3://fractal-protocol-shared-libs/$LIB" - | tar -xz -C "$SHARED_LIBS_DIR"

    # Copy Windows files
    if [[ "$OS" =~ "Windows" ]]; then
        cp "$SHARED_LIBS_DIR/share/64/Windows"/* "$CLIENT_DIR"
        cp "$SHARED_LIBS_DIR/share/64/Windows"/* "$SERVER_DIR"
    fi

    rm -r "$SHARED_LIBS_DIR"
fi

###############################
# Copy shared libs
###############################

# Copy macOS files
if [[ "$OS" == "Darwin" ]]; then
    cp "$SOURCE_DIR/lib/64/ffmpeg/Darwin"/* "$CLIENT_DIR"
fi

###############################
# Download SDL2 headers
###############################

# If the include/SDL2 directory doesn't exist, make it and fill it
# Or, if the lib has updated, refill the directory
LIB="fractal-sdl2-headers.tar.gz"
SDL_DIR="$SOURCE_DIR/include/SDL2"
if [[ ! -d "$SDL_DIR" ]] || has_updated "$LIB"; then
    mkdir -p "$SDL_DIR"
    aws s3 cp --only-show-errors "s3://fractal-protocol-shared-libs/$LIB" - | tar -xz -C "$SDL_DIR"

    # Pull all SDL2 include files up a level and delete encapsulating folder
    mv "$SDL_DIR/include"/* "$SDL_DIR"
    rmdir "$SDL_DIR/include"
fi

###############################
# Download SDL2 libraries
###############################

# Select SDL lib dir and SDL lib targz name based on OS
SDL_LIB_DIR="$SOURCE_DIR/lib/64/SDL2/$OS"
if [[ "$OS" =~ "Windows" ]]; then
    SDL_LIB="fractal-windows-sdl2-static-lib.tar.gz"
elif [[ "$OS" == "Darwin" ]]; then
    SDL_LIB="fractal-macos-sdl2-static-lib.tar.gz"
elif [[ "$OS" == "Linux" ]]; then
    SDL_LIB="fractal-linux-sdl2-static-lib.tar.gz"
fi

# Check if SDL_LIB has updated, and if so, create the dir and copy the libs into the source dir
if has_updated "$SDL_LIB"; then
    mkdir -p "$SDL_LIB_DIR"
    aws s3 cp --only-show-errors "s3://fractal-protocol-shared-libs/$SDL_LIB" - | tar -xz -C "$SDL_LIB_DIR"
fi

###############################
echo "Download Completed"
