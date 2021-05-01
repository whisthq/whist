#!/bin/bash

set -Eeuo pipefail

echo $CACHED_COMMIT_REF $COMMIT_REF $PWD
git diff $CACHED_COMMIT_REF $COMMIT_REF ||:
echo $URL $BRANCH $HEAD $PULL_REQUEST

export REACT_APP_VERSION=$COMMIT_REF && npm run build && rm -rf /opt/build/cache
