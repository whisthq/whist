#!/bin/bash

# This script downloads pre-built Fractal protocol libraries from AWS S3, on all OSes

# Exit on errors and missing environment variables
set -Eeuo pipefail

###############################
# Detect Parameters
###############################

OS="$1"
CLIENT_DIR="$2"
SERVER_DIR="$3"
DEST_DIR="$4"
CACHE_DIR="$5"
mkdir -p "$CLIENT_DIR"
mkdir -p "$SERVER_DIR"
mkdir -p "$CACHE_DIR"

echo "Downloading Protocol Libraries"

CACHE_FILE="$CACHE_DIR/.libcache"
touch "$CACHE_FILE"
# NOTE: Should be listed _first_ in an || statement, so that it updates the timestamp without getting short-circuited
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
# Download SDL2 headers
###############################

# If the include/SDL2 directory doesn't exist, make it and fill it
# Or, if the lib has updated, refill the directory
LIB="fractal-sdl2-headers.tar.gz"
SDL_DIR="$DEST_DIR/include/SDL2"
if has_updated "$LIB" || [[ ! -d "$SDL_DIR" ]]; then
  rm -rf "$SDL_DIR"
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
SDL_LIB_DIR="$DEST_DIR/lib/64/SDL2/$OS"
if [[ "$OS" =~ "Windows" ]]; then
  SDL_LIB="fractal-windows-sdl2-static-lib.tar.gz"
elif [[ "$OS" == "Darwin" ]]; then
  SDL_LIB="fractal-macos-sdl2-static-lib.tar.gz"
elif [[ "$OS" == "Linux" ]]; then
  SDL_LIB="fractal-linux-sdl2-static-lib.tar.gz"
fi

# Check if SDL_LIB has updated, and if so, create the dir and copy the libs into the source dir
if has_updated "$SDL_LIB"; then
  rm -rf "$SDL_LIB_DIR"
  mkdir -p "$SDL_LIB_DIR"
  aws s3 cp --only-show-errors "s3://fractal-protocol-shared-libs/$SDL_LIB" - | tar -xz -C "$SDL_LIB_DIR"
fi

###############################
# Download Sentry headers
###############################

# If the include/sentry directory doesn't exist, make it and fill it
# Or, if the lib has updated, refill the directory
LIB="21-fractal-sentry-headers.tar.gz"
SENTRY_DIR="$DEST_DIR/include/sentry"
if has_updated "$LIB" || [[ ! -d "$SENTRY_DIR" ]]; then
  rm -rf "$SENTRY_DIR"
  mkdir -p "$SENTRY_DIR"
  aws s3 cp --only-show-errors "s3://fractal-protocol-shared-libs/$LIB" - | tar -xz -C "$SENTRY_DIR"

  # Pull all SDL2 include files up a level and delete encapsulating folder
  mv "$SENTRY_DIR/include"/* "$SENTRY_DIR"
  rmdir "$SENTRY_DIR/include"
fi

###############################
# Download Sentry libraries
###############################

# Select sentry lib dir and sentry lib targz name based on OS
SENTRY_LIB_DIR="$DEST_DIR/lib/64/sentry/$OS"
if [[ "$OS" =~ "Windows" ]]; then
  SENTRY_LIB="21-fractal-windows-sentry-shared-lib.tar.gz"
elif [[ "$OS" == "Darwin" ]]; then
  SENTRY_LIB="21-fractal-macos-sentry-shared-lib.tar.gz"
elif [[ "$OS" == "Linux" ]]; then
  SENTRY_LIB="21-fractal-linux-sentry-shared-lib.tar.gz"
fi

# Check if SENTRY_LIB has updated, and if so, create the dir and copy the libs into the source dir
if has_updated "$SENTRY_LIB"; then
  rm -rf "$SENTRY_LIB_DIR"
  mkdir -p "$SENTRY_LIB_DIR"
  aws s3 cp --only-show-errors "s3://fractal-protocol-shared-libs/$SENTRY_LIB" - | tar -xz -C "$SENTRY_LIB_DIR"
fi

###############################
# Download OpenSSL headers
###############################

# If the include/openssl directory doesn't exist, make it and fill it
# Or, if the lib has updated, refill the directory
LIB="21-fractal-libcrypto-headers.tar.gz"
OPENSSL_DIR="$DEST_DIR/include/openssl"
if has_updated "$LIB" || [[ ! -d "$OPENSSL_DIR" ]]; then
  rm -rf "$OPENSSL_DIR"
  mkdir -p "$OPENSSL_DIR"
  aws s3 cp --only-show-errors "s3://fractal-protocol-shared-libs/$LIB" - | tar -xz -C "$OPENSSL_DIR"

  # Pull all OpenSSL include files up a level and delete encapsulating folder
  mv "$OPENSSL_DIR/include"/* "$OPENSSL_DIR"
  rmdir "$OPENSSL_DIR/include"
fi

###############################
# Download OpenSSL libraries
###############################

# Select OpenSSL lib dir and OpenSSL lib targz name based on OS
OPENSSL_LIB_DIR="$DEST_DIR/lib/64/openssl/$OS"
if [[ "$OS" =~ "Windows" ]]; then
  OPENSSL_LIB="21-fractal-windows-libcrypto-static-lib.tar.gz"
elif [[ "$OS" == "Darwin" ]]; then
  OPENSSL_LIB="21-fractal-macos-libcrypto-static-lib.tar.gz"
elif [[ "$OS" == "Linux" ]]; then
  OPENSSL_LIB="21-fractal-linux-libcrypto-static-lib.tar.gz"
fi

# Check if OPENSSL_LIB has updated, and if so, create the dir and copy the libs into the source dir
if has_updated "$OPENSSL_LIB"; then
  rm -rf "$OPENSSL_LIB_DIR"
  mkdir -p "$OPENSSL_LIB_DIR"
  aws s3 cp --only-show-errors "s3://fractal-protocol-shared-libs/$OPENSSL_LIB" - | tar -xz -C "$OPENSSL_LIB_DIR"
fi

###############################
# Download FFmpeg headers
###############################

# If the include/ffmpeg directory doesn't exist, make it and fill it
# Or, if the lib has updated, refill the directory
LIB="fractal-ffmpeg-headers.tar.gz"
FFMPEG_DIR="$DEST_DIR/include/ffmpeg"
if has_updated "$LIB" || [[ ! -d "$FFMPEG_DIR" ]]; then
  rm -rf "$FFMPEG_DIR"
  mkdir -p "$FFMPEG_DIR"
  aws s3 cp --only-show-errors "s3://fractal-protocol-shared-libs/$LIB" - | tar -xz -C "$FFMPEG_DIR"

  mv "$FFMPEG_DIR/include/"/* "$FFMPEG_DIR"
  rmdir "$FFMPEG_DIR/include"
fi

###############################
# Download FFmpeg libraries
###############################

# Select FFmpeg lib dir and targz name
FFMPEG_LIB_DIR="$DEST_DIR/lib/64/ffmpeg"
if [[ "$OS" =~ "Windows" ]]; then
  FFMPEG_LIB="fractal-windows-ffmpeg-shared-lib.tar.gz"
elif [[ "$OS" == "Darwin" ]]; then
  FFMPEG_LIB="fractal-macos-ffmpeg-shared-lib.tar.gz"
elif [[ "$OS" == "Linux" ]]; then
  FFMPEG_LIB="fractal-linux-ffmpeg-shared-lib.tar.gz"
fi

# Check if FFMPEG_LIB has updated, and if so, create the dir and copy the libs into the source dir
if has_updated "$FFMPEG_LIB"; then
  rm -rf "$FFMPEG_LIB_DIR"
  mkdir -p "$FFMPEG_LIB_DIR"
  aws s3 cp --only-show-errors "s3://fractal-protocol-shared-libs/$FFMPEG_LIB" - | tar -xz -C "$FFMPEG_LIB_DIR"
fi

###############################
echo "Download Completed"
