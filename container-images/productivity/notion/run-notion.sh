#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

# Close all instances of Google Chrome (to simplify login redirection from the browser to Notion)
if pgrep chrome; then
    pkill chrome
fi

# Start Notion
pushd "/opt/notion-app/" >/dev/null || exit 1
./electron app $@
popd >/dev/null || exit 1
