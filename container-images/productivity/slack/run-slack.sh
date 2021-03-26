#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

# Close all instances of Google Chrome (to simplify login redirection from the browser to Slack)
if pgrep chrome; then
    pkill chrome
fi

# Start Slack
exec /usr/lib/slack/slack $@
