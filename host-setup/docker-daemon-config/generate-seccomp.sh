#!/bin/bash

set -Eeuo pipefail

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

moby_commit=$(sudo docker version --format "{{.Server.GitCommit}}")
seccomp_url="https://github.com/moby/moby/raw/$moby_commit/profiles/seccomp/default.json"

cd "$DIR"
curl -L $seccomp_url | jq -f merge.jq --slurpfile patch seccomp-patch.json > seccomp.json
