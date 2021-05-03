#!/bin/bash

set -Eeuo pipefail

echo "Logging Netlify Build Environment (to debug netlify.toml)"
printf "COMMIT_REF %s | CACHED_COMMIT_REF %s | PWD %s\n" \
        $COMMIT_REF     $CACHED_COMMIT_REF     $PWD
printf "URL %s | BRANCH %s | HEAD %s | PULL_REQUEST %s\n" \
        $URL     $BRANCH     $HEAD     $PULL_REQUEST

npm run build && rm -rf /opt/build/cache
