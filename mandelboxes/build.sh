#!/bin/bash

set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Working directory is whist/mandelboxes/
cd "$DIR"

# In dev mode, we copy the protocol at the last step of each target mandelbox image, to speed up development time, and
# we use the Debug cmake build target. In prod mode, we copy the protocol at the last step of the base image for
# consistency, and use the Release cmake build type. Generally speaking, dev mode is good for working on the protocol
# and prod mode is good for CI and working on the mandelbox itself.
mode=dev
cmake_build_type_opt=""
for arg in "$@"; do
  case $arg in
    --dev|-d)
      mode=dev
      ;;
    --prod|-p)
      mode=prod
      ;;
    --metrics|-M)
      mode=metrics
      ;;
    --release-protocol)
      cmake_build_type_opt=Release
      ;;
    --relwithdebinfo-protocol)
      cmake_build_type_opt=RelWithDebInfo
      ;;
    --debug-protocol)
      cmake_build_type_opt=Debug
      ;;
    *)
      python_args+=("$arg")
      ;;
  esac
done

# If dev mode, delete dangling/untagged docker images more than a week old.
# This will ensure that instances don't run out of space as fast!
if [[ "$mode" == "dev" ]]; then
  echo "Pruning stale dangling docker images..."
  docker image prune --filter "until=48h" --force
  docker builder prune --filter "until=48h" --force
fi

# Nuke the build-assets temp directory
rm -rf base/build-assets/build-temp && mkdir base/build-assets/build-temp

# Build and copy the protocol
if [[ "$mode" == "dev" ]]; then
  cmake_build_type=Debug
elif [[ "$mode" == "metrics" ]]; then
  cmake_build_type=Metrics
  mode=dev
else
  cmake_build_type=Release
fi
if [[ -n "$cmake_build_type_opt" ]]; then
  cmake_build_type=$cmake_build_type_opt
fi

# Build the WhistClient if we are building the development/client mandelbox. Otherwise, build the WhistServer
if [[ "${python_args[0]}" == "development/client" ]]; then
  echo "Building $cmake_build_type WhistClient..."
  ../protocol/build_protocol_targets.sh --cmakebuildtype=$cmake_build_type WhistClient
  ./scripts/copy_protocol_build.sh base/build-assets/build-temp WhistClient
else
  echo "Building $cmake_build_type WhistServer..."
  ../protocol/build_protocol_targets.sh --cmakebuildtype=$cmake_build_type WhistServer
  ./scripts/copy_protocol_build.sh base/build-assets/build-temp WhistServer
fi

# Copy the Nvidia driver installer
echo "Fetching Nvidia driver installer..."
mkdir base/build-assets/build-temp/nvidia-driver
../host-setup/get-nvidia-driver-installer.sh
mv nvidia-driver-installer.run base/build-assets/build-temp/nvidia-driver

# Copy the fonts used in Whist
echo "Fetching the Whist fonts..."
mkdir base/build-assets/build-temp/fonts
aws s3 cp --only-show-errors s3://whist-fonts base/build-assets/build-temp/fonts --recursive

# Bundle these build assets into a cached Docker image
echo "Bundling build assets..."
docker build -t whist/build-assets:default -f base/build-assets/Dockerfile.20 --target default base/build-assets -q > /dev/null
docker build -t whist/build-assets:protocol -f base/build-assets/Dockerfile.20 --target protocol base/build-assets -q > /dev/null

# Now, our Dockerfiles can copy over these files using
# COPY --from=whist/build-assets:default most of the time,
# and whist/build-assets:protocol when they determine that
# they want to copy the protocol based on the mode.

python3 ./scripts/build_mandelbox_image.py "${python_args[@]}" --mode="$mode"
