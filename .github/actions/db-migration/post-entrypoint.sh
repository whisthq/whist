#!/bin/bash

set -Eeuo pipefail

# Here, we reference a STATE_CODE and STATE_DIFF values, which should contain
# the result from entrypoint.sh.

# You must prefix variables with STATE_ to pass them between the
# pre-entrypoint.sh, entrypoint.sh, and post-entrypoint.sh processes.

# To pass a value to the GitHub Actions outputs, you must echo a string formatted
# like the strings below, where name= is the name of the output as it appears in
# action.yml.

echo "::set-output name=code::$(echo $STATE_CODE)"
echo "::set-output name=diff::$(echo $STATE_DIFF)"
