#!/bin/bash

# This script walks the folder tree it is run in, finds all Dockerfile.20 it contains and
# formats their path properly for pushing to GitHub Container Registry.

# Exit on subcommand errors
set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR/.."

# Find all the Dockerfiles in this directory, match to "./[PATH]/Dockerfile.20", and replace '\n' with ' '
find . -name "Dockerfile.20" | sed 's/^\.\/\(.*\)\/Dockerfile\.20$/\1/g' | tr '\n' ' '

