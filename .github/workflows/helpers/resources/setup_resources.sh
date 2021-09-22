#!/bin/bash

# Adds the Resources folder to python's path.

set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Working directory is fractal/.github/workflows/helpers
cd "$DIR/.."

# Create a temporary folder, and copy the resources modules to that folder.
# We copy instead of move to avoid mutating our repository.
# The point is to ensure that this package can be imported even if our git repo changes.
tmpfolder=`mktemp -d`
cp -r resources $tmpfolder

# Add the temp folder to our Python path, so we can import from resources.
# The fancy syntax means "$PYTHONPATH:" if it's defined, else ""
echo "PYTHONPATH=${PYTHONPATH:+${PYTHONPATH}:}$tmpfolder" >> $GITHUB_ENV
