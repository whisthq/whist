#!/bin/bash

# This script starts the symlinked fractal-application as the fractal user.

# Exit on subcommand errors
set -Eeuo pipefail

# Write the PID to a file
FRACTAL_APPLICATION_PID_FILE="/home/fractal/fractal-application-pid"
echo $$ > $FRACTAL_APPLICATION_PID_FILE

# Wait for the PID file to have been removed
while [ -f "$FRACTAL_APPLICATION_PID_FILE" ]
do
  sleep 0.1
done

# Pass JSON transport settings as environment variables
export DARK_MODE=$DARK_MODE
export RESTORE_LAST_SESSION=$RESTORE_LAST_SESSION
export TZ=$TZ #TZ variable automatically adjusts the timezone (https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html)


# Start the application that this mandelbox runs
exec fractal-application
