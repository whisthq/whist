#!/bin/bash

# Configure GHA to import the notifications package and optionally install dependencies.
# Note that installed dependencies will be reset if you call the configure Python action
# after running this script.
# Arguments
# ${1}: true if and only if the script should install the requirements for the package. Default false.

set -Eeuo pipefail

INSTALL_DEPS=${1-"false"}

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Working directory is whist/.github/workflows/helpers
cd "$DIR/.."

# Install the Python dependencies only if the invoker requests them
# we do this to minimize installing dependencies we do not need.
# For example, the Slack bot only requires the `requests` library,
# which is built into GHA VMs. Users of the library don't need to
# install/import any other dependency, which minimizes the chance of
# things going wrong in GHA
if [[ $INSTALL_DEPS == "true" ]]; then
  pip install -r notifications/requirements.txt
fi

# Create a temporary folder, and copy the notifications module to that folder.
# We copy instead of move to avoid mutating our repository.
# The point is to ensure that this package can be imported even if our git repo changes.
tmpfolder=$(mktemp -d)
cp -r notifications "$tmpfolder"

# Add the temp folder to our Python path, so we can import from notifications.
# The fancy syntax means "$PYTHONPATH:" if it's defined, else ""
echo "PYTHONPATH=${PYTHONPATH:+${PYTHONPATH}:}$tmpfolder" >> "$GITHUB_ENV"
