#!/bin/bash

# This script lints our mandelbox Dockerfiles using hadolint.

set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Working directory is whist
cd "$DIR/.."

exec docker run -v $PWD:/whist:ro --workdir=/whist --rm -i hadolint/hadolint:2.8.0-debian hadolint mandelboxes/*/{.,*}/Dockerfile.20
