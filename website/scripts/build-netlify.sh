#!/bin/bash

set -Eeuo pipefail

export REACT_APP_VERSION=$COMMIT_REF && npm run build && rm -rf /opt/build/cache