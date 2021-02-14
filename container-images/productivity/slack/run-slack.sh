#!/bin/bash

# Exit on errors and missing environment variables
set -Eeuo pipefail

# Close all instances of Google Chrome (to simplify login redirection from the browser to Slack)
if pgrep chrome; then
    pkill chrome
fi

# Start Slack
exec /usr/lib/slack/slack $@
