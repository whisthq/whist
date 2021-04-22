#!/bin/bash

# Install dependencies and configure Python to import the notifications package.
# Note that this will be reset if you call the configure Python action afterwards.

set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Working directory is fractal/.github/workflows/helpers
cd "$DIR/.."

# Install the Python dependencies
pip install -r notifications/requirements.txt

# Create a temporary folder, and copy the notifications module to that folder.
# We copy instead of move to avoid mutating our repository.
# The point is to ensure that this package can be imported even if our git repo changes.
tmpfolder=`mktemp -d`
cp -r notifications $tmpfolder

# Add the temp folder to our python path, so we can import from notifications.
echo "PYTHONPATH=$PYTHONPATH:$tmpfolder" >> $GITHUB_ENV
