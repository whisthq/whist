#!/bin/bash

set -Eeuo pipefail

# Retrieve source directory of this script
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR"

# We want only _our_ CMakeLists.txt. Fortunately,
# `fd` is a lightning-fast `find` replacement which
# ignores `.gitignore`d files by default!
CMAKELISTS_FILES=$(fd --glob 'CMakeLists.txt')
CMAKE_HELPER_FILES=$(fd --glob '*.cmake')

echo "Linting the following files:"
echo "$CMAKELISTS_FILES"
echo "$CMAKE_HELPER_FILES"

cmakelint --config=".cmakelintrc" $CMAKELISTS_FILES $CMAKE_HELPER_FILES
