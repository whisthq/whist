#!/bin/bash

set -Eeuo pipefail

# In the GitHub Actions runner, this script will be run with the monorepo
# root as the working directory. Our file path to the Python script must be
# relative to the monorepo root folder.

# We forward all the arguments passed to the docker run proces to this script.
# You can see the arguments that will be passed to this script in actions.yml
# for this GitHub Action.
STATE_CONFIG=$(python .github/actions/monorepo-config/main.py "$@")

echo "$STATE_CONFIG"
