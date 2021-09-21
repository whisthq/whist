#!/bin/bash

# Configure GHA to import the AWS and resources packages.

set -Eeuo pipefail

# Create a temporary folder, and copy the aws and resources modules to that folder.
# We copy instead of move to avoid mutating our repository.
# The point is to ensure that this package can be imported even if our git repo changes.
tmpfolder=`mktemp -d`
cp -r aws $tmpfolder
cp -r resources $tmpfolder

# Add the temp folder to our Python path, so we can import from aws and resources.
# The fancy syntax means "$PYTHONPATH:" if it's defined, else ""
echo "PYTHONPATH=${PYTHONPATH:+${PYTHONPATH}:}$tmpfolder" >> $GITHUB_ENV
