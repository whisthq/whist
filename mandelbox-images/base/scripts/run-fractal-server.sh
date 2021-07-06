#!/bin/bash

# This script starts the Fractal protocol server, and assumes that it is
# being run within a Fractal mandelbox.

# Exit on subcommand errors
set -Eeuo pipefail

# Set/Retrieve Mandelbox parameters
FRACTAL_MAPPINGS_DIR=/fractal/resourceMappings
USER_CONFIGS_DIR=/fractal/userConfigs
IDENTIFIER_FILENAME=hostPort_for_my_32262_tcp
PRIVATE_KEY_FILENAME=/usr/share/fractal/private/aes_key
SENTRY_ENV_FILENAME=/usr/share/fractal/private/sentry_env
TIMEOUT_FILENAME=$FRACTAL_MAPPINGS_DIR/timeout
FRACTAL_APPLICATION_PID_FILE=/home/fractal/fractal-application-pid
LOG_FILENAME=/usr/share/fractal/server.log

# Define a string-format identifier for this mandelbox
IDENTIFIER=$(cat $FRACTAL_MAPPINGS_DIR/$IDENTIFIER_FILENAME)

# Create list of command-line arguments to pass to the Fractal protocol server
OPTIONS=""

# Send in AES private key, if set
if [ -f "$PRIVATE_KEY_FILENAME" ]; then
    export FRACTAL_AES_KEY=$(cat $PRIVATE_KEY_FILENAME)
    OPTIONS="$OPTIONS --private-key=$FRACTAL_AES_KEY"
fi

# Send in Sentry environment, if set
if [ -f "$SENTRY_ENV_FILENAME" ]; then
    export SENTRY_ENV=$(cat $SENTRY_ENV_FILENAME)
    OPTIONS="$OPTIONS --environment=$SENTRY_ENV"
fi

# Send in timeout, if set
if [ -f "$TIMEOUT_FILENAME" ]; then
    export TIMEOUT=$(cat $TIMEOUT_FILENAME)
    OPTIONS="$OPTIONS --timeout=$TIMEOUT"
fi

# This function is called whenever the script exits, whether that is because we
# reach the end of this file (because either FractalServer or the Fractal
# application died), or there was an error somewhere else in the script.
function cleanup {
    echo "cleanup() called! Shutting down the mandelbox..."

    # Make sure we shutdown the mandelbox when this script exits
    sudo shutdown now
}

# Make sure `cleanup` gets called on script exit.
trap cleanup EXIT ERR

# Start the application that this mandelbox runs.
/usr/share/fractal/run-as-fractal-user.sh "/usr/bin/run-fractal-application.sh" &
fractal_application_runuser_pid=$!

# Wait for run-fractal-application.sh to write PID to file
until [ -f "$FRACTAL_APPLICATION_PID_FILE" ]
do
    sleep 0.1
done
fractal_application_pid=$(cat $FRACTAL_APPLICATION_PID_FILE)
rm $FRACTAL_APPLICATION_PID_FILE

echo "Now sleeping until there are X clients..."

# Wait until the application has created its display before launching FractalServer.
#    This prevents a black no input window from appearing when a user connects.
until [ $(xlsclients -display :10 | wc -l) != 0 ]
do
    sleep 0.1
done

echo "Done sleeping until there are X clients..."
echo "done" > $FRACTAL_MAPPINGS_DIR/done_sleeping_until_X_clients
sync # Necessary so that even if the container exits very soon the host service sees the file written.

# Send in identifier
OPTIONS="$OPTIONS --identifier=$IDENTIFIER"

# The point of the named pipe redirection is so that $! will give us the PID of FractalServer, not of tee.
/usr/share/fractal/FractalServer $OPTIONS > >(tee $LOG_FILENAME) &
fractal_server_pid=$!

# Wait for either fractal-application or FractalServer to exit (both backgrounded processes).

# TODO: once our mandelboxes have bash 5.1 we will be able to deduce _which_
# application exited with the `-p` flag to `wait`.
wait -n
echo "Either FractalServer or fractal-application exited with code $?"
echo "FractalServer PID: $fractal_server_pid"
echo "runuser fractal-application PID: $fractal_application_runuser_pid"
echo "fractal-application PID: $fractal_application_pid"
echo "Remaining job PIDs: $(jobs -p)"

# Kill whatever is still running of FractalServer and fractal-application, with SIGTERM.
kill $fractal_application_pid ||:
kill $fractal_server_pid ||:

# Wait for fractal-application to finish terminating, ignoring exit code (since
# Firefox, for instance, exits with a nonzero code on SIGTERM).
wait $fractal_application_runuser_pid ||:

echo "Both fractal-application and FractalServer have exited."

# We now pass control over to `cleanup`, since we've reached the end of the script.
