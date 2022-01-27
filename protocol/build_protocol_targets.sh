#!/bin/bash

set -Eeuo pipefail

# Retrieve source directory of this script
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR"


# `usage` is the help function we provide.
usage() {
  cat << EOF
USAGE:
  build_protocol_targets.sh [--cmakebuildtype=BUILDTYPE ] [ --nodownloadbinaries ] [ --cmakesetCI ] TARGETS...
This script builds the specified protocol target(s) inside the protocol-builder
Docker container.
The optional argument \`--cmakebuildtype\` can be specified
to be either \`Debug\` or \`Release\`. It controls the \`CMAKE_BUILD_TYPE\`
build parameter. It defaults to \`Debug\`.
The optional argument \`--nodownloadbinaries\`, when provided, tells \`cmake\`
not to download the libraries used to build the Whist protocol from S3.
The optional argument \`--cmakesetCI\`, if provided, is used to set
\`-DCI=TRUE\` in the cmake configuration.
EOF

  # We set a nonzero exit code so that CI doesn't accidentally only run `usage` and think it succeeded.
  exit 2
}

# Parse arguments (derived from https://stackoverflow.com/a/7948533/2378475)
# I'd prefer not to have the short arguments at all, but getopt only uses short arguments
TEMP=$(getopt -o h --long help,usage,cmakebuildtype:,nodownloadbinaries,cmakesetCI,sanitize: -n 'build_protocol_targets.sh' -- "$@")
eval set -- "$TEMP"

CMAKE_BUILD_TYPE=Debug
CMAKE_DOWNLOAD_BINARIES=ON
CMAKE_SET_CI=FALSE
CMAKE_SANITIZE=OFF
while true; do
  case "$1" in
    -h | --help | --usage ) usage ;;
    --cmakebuildtype ) CMAKE_BUILD_TYPE="$2"; shift 2 ;;
    --nodownloadbinaries ) CMAKE_DOWNLOAD_BINARIES=OFF; shift ;;
    --cmakesetCI ) CMAKE_SET_CI=TRUE; shift ;;
    --sanitize ) CMAKE_SANITIZE="$2"; shift 2 ;;
    -- ) shift; break ;;
    * ) echo "We should never be able to get into this argument case! Unknown argument passed in: $1"; exit -1 ;;
  esac
done
TARGETS="$@"

if [[ -z "$TARGETS" ]]; then
  usage
fi

# Set user if not already set
USER=$(whoami)

# Build protocol-builder Docker image
docker build . \
  --build-arg uid=$(id -u ${USER}) \
  -f Dockerfile \
  -t whisthq/protocol-builder

DOCKER_USER="whist-builder"

# We mount .aws directory so that awscli in download-binaries.sh works
MOUNT_AWS=""
if [[ -d "$HOME/.aws" ]]; then
  MOUNT_AWS="--mount type=bind,source=$HOME/.aws,destination=/home/$DOCKER_USER/.aws,readonly"
fi

# If there is a TTY (yes if dev instance, no if CI),
# run with -it so that Ctrl+C will cancel the command.
if [ -t 0 ] && [ -t 1 ]; then
  DOCKER_IT_FLAG="-it"
else
  DOCKER_IT_FLAG=""
fi

# We also mount entire ./whist directory so that git works for git revision
docker run \
  --rm \
  $DOCKER_IT_FLAG \
  --env AWS_ACCESS_KEY_ID --env AWS_SECRET_ACCESS_KEY --env AWS_DEFAULT_REGION --env AWS_DEFAULT_OUTPUT --env GITHUB_SHA --env CODECOV_TOKEN \
  --mount type=bind,source=$(cd ..; pwd),destination=/workdir \
  $MOUNT_AWS \
  --name whist-protocol-builder-$(date +"%s") \
  --user "$DOCKER_USER" \
  whisthq/protocol-builder \
  bash -c "\
    cd protocol &&                                      \
    mkdir -p build-docker &&                            \
    cd build-docker &&                                  \
    cmake                                               \
        -S ..                                           \
        -B .                                            \
        -DDOWNLOAD_BINARIES=${CMAKE_DOWNLOAD_BINARIES}  \
        -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}         \
        -D CHECK_CI=${CMAKE_SET_CI}                     \
        -D SANITIZE=${CMAKE_SANITIZE} &&                \
    make -j ${TARGETS}                                  \
  "
