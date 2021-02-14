#!/bin/bash

# Exit on errors and missing environment variables
set -Eeuo pipefail

# Close all instances of Google Chrome (to simplify login redirection from the browser to Figma)
if pgrep chrome; then
    pkill chrome
fi

# Start Figma
pushd "/opt/figma/" >/dev/null || exit 1
exec ./electron app.asar $@
popd >/dev/null || exit 1
