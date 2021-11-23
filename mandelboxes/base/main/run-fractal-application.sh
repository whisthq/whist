#!/bin/bash

# This script starts the symlinked fractal-application as the fractal user.

# Exit on subcommand errors
set -Eeuo pipefail

# Write the PID to a file
WHIST_APPLICATION_PID_FILE="/home/fractal/fractal-application-pid"
echo $$ > $WHIST_APPLICATION_PID_FILE

# Wait for the PID file to have been removed
while [ -f "$WHIST_APPLICATION_PID_FILE" ]
do
  sleep 0.1
done

# Pass JSON transport settings as environment variables
export DARK_MODE=$DARK_MODE
export RESTORE_LAST_SESSION=$RESTORE_LAST_SESSION
#TZ variable automatically adjusts the timezone (https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html)
export TZ=$TZ
export INITIAL_URL=$INITIAL_URL

# Start the application that this mandelbox runs
exec fractal-application
