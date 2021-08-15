#!/bin/bash

set -Eeuo pipefail

echo "Logging Netlify Build Environment (to debug netlify.toml)"
printf "COMMIT_REF %s | CACHED_COMMIT_REF %s | PWD %s\n" \
  $COMMIT_REF     $CACHED_COMMIT_REF     $PWD
printf "URL %s | BRANCH %s | HEAD %s | PULL_REQUEST %s\n" \
  $URL     $BRANCH     $HEAD     $PULL_REQUEST

yarn build && rm -rf /opt/build/cache

# We need to let the React app handle all routing login.
# Without this, directly navigating to fractal.co/about fails.
echo '/* /index.html 200' > build/_redirects
