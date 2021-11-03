#!/bin/bash

set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Working directory is fractal/mandelboxes/
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

# Nuke the build-assets temp directory
rm -rf base/build-assets/build-temp && mkdir base/build-assets/build-temp

# Build and copy the protocol
if [[ "$mode" == "dev" ]]; then
  cmake_build_type=Debug
else
  cmake_build_type=Release
fi
if [[ ! -z "$cmake_build_type_opt" ]]; then
  cmake_build_type=$cmake_build_type_opt
fi
echo "Building $cmake_build_type FractalServer..."
../protocol/build_protocol_targets.sh --cmakebuildtype=$cmake_build_type FractalServer
./helper_scripts/copy_protocol_build.sh base/build-assets/build-temp

# Copy the nvidia driver installer
mkdir base/build-assets/build-temp/nvidia-driver
../host-setup/get-nvidia-driver-installer.sh && mv nvidia-driver-installer.run base/build-assets/build-temp/nvidia-driver

# Bundle these build assets into a cached docker image
docker build -t fractal/build-assets:default -f base/build-assets/Dockerfile.20 --target default base/build-assets -q
docker build -t fractal/build-assets:protocol -f base/build-assets/Dockerfile.20 --target protocol base/build-assets -q

# Now, our Dockerfiles can copy over these files using
# COPY --from=fractal/build-assets:default most of the time,
# and fractal/build-assets:protocol when they determine that
# they want to copy the protocol based on the mode.

python3 ./helper_scripts/build_mandelbox_image.py "${python_args[@]}" --mode="$mode"
