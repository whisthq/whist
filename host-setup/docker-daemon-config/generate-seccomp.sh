#!/bin/bash

set -Eeuo pipefail

# Retrieve source directory of this script
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR"

moby_commit=$(sudo docker version --format "{{.Server.GitCommit}}")
seccomp_url="https://github.com/moby/moby/raw/$moby_commit/profiles/seccomp/default.json"

curl -L $seccomp_url | jq -f merge.jq --slurpfile patch seccomp-patch.json > seccomp.json
