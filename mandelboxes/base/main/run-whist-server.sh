#!/bin/bash

# This script starts the Whist protocol server and the Whist application.

# Enable Sentry bash error handler, this will catch errors if `set -e` is set in a Bash script
WHIST_PRIVATE_DIR=/usr/share/whist/private
SENTRY_ENV_FILENAME=$WHIST_PRIVATE_DIR/sentry_env
case $(cat $SENTRY_ENV_FILENAME) in
  dev|staging|prod)
    export SENTRY_ENVIRONMENT=${SENTRY_ENV}
    eval "$(sentry-cli bash-hook)"
    ;;
  *)
    echo "Sentry environment not set, skipping Sentry error handler"
    ;;
esac

# Exit on subcommand errors
set -Eeuo pipefail

# Set/Retrieve Mandelbox parameters
WHIST_MAPPINGS_DIR=/whist/resourceMappings
IDENTIFIER_FILENAME=hostPort_for_my_32262_tcp
PRIVATE_KEY_FILENAME=$WHIST_PRIVATE_DIR/aes_key
COOKIE_FILE_FILENAME=$WHIST_PRIVATE_DIR/user_cookies_file
BOOKMARK_FILE_FILENAME=$WHIST_PRIVATE_DIR/user_bookmarks_file
EXTENSION_FILENAME=$WHIST_PRIVATE_DIR/extensions_file
USER_UPLOAD_TARGET_FILENAME=$WHIST_PRIVATE_DIR/user_target
TIMEOUT_FILENAME=$WHIST_MAPPINGS_DIR/timeout
WHIST_APPLICATION_PID_FILE=/home/whist/whist-application-pid
PROTOCOL_LOG_FILENAME=/usr/share/whist/server.log
TELEPORT_LOG_FILENAME=/usr/share/whist/teleport.log

# Define a string-format identifier for this mandelbox
IDENTIFIER=$(cat $WHIST_MAPPINGS_DIR/$IDENTIFIER_FILENAME)

# Create list of command-line arguments to pass to the Whist protocol server
OPTIONS=""

# Send in AES private key, if set
if [ -f "$PRIVATE_KEY_FILENAME" ]; then
  export WHIST_AES_KEY=$(cat $PRIVATE_KEY_FILENAME)
  OPTIONS="$OPTIONS --private-key=$WHIST_AES_KEY"
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

# Set cookies file, if file exists
if [ -f "$COOKIE_FILE_FILENAME" ]; then
  export WHIST_INITIAL_USER_COOKIES_FILE=$(cat $COOKIE_FILE_FILENAME)
fi

# Set user upload target, if file exists
if [ -f "$USER_UPLOAD_TARGET_FILENAME" ]; then
  export WHIST_COOKIE_UPLOAD_TARGET=$(cat $USER_UPLOAD_TARGET_FILENAME)
fi

if [ -f "$BOOKMARK_FILE_FILENAME" ]; then
  export WHIST_INITIAL_USER_BOOKMARKS_FILE=$(cat $BOOKMARK_FILE_FILENAME)
fi

if [ -f "$EXTENSION_FILENAME" ] && [ -f "$(cat $EXTENSION_FILENAME)" ]; then
  IN="$(cat $(cat $EXTENSION_FILENAME))"
  extensions=(${IN//,/ })
  for extension in "${extensions[@]}"
  do
    #  Install user extensions
    /usr/bin/install-extension.sh $extension
  done
  rm $(cat $EXTENSION_FILENAME)
fi

# We use named pipe redirection for consistency with our WhistServer launch setup
# &> redirects both stdout and stdin together; shorthand for '> XYZ 2>&1'
/usr/share/whist/run-as-whist-user.sh "/usr/bin/run-whist-teleport.sh" &> >(tee $TELEPORT_LOG_FILENAME) &

# This function is called whenever the script exits, whether that is because we
# reach the end of this file (because either WhistServer or the Whist
# application died), or there was an error somewhere else in the script.
function cleanup {
  echo "cleanup() called! Shutting down the mandelbox..."

  # Make sure we shutdown the mandelbox when this script exits
  sudo shutdown now
}

export ENV_NAME=$(cat $SENTRY_ENV_FILENAME)
if [ "$ENV_NAME" != "localdev" ]; then
  # Make sure `cleanup` gets called on script exit in all environments except localdev.
  trap cleanup EXIT ERR
fi

# Imports user browser data if file exists
python3 /usr/share/whist/import_user_browser_data.py

if [ -n "${WHIST_INITIAL_USER_COOKIES_FILE+1}" ] && [ -f "$WHIST_INITIAL_USER_COOKIES_FILE" ]; then
  # Remove temporary file containing the user's intial cookies
  rm $WHIST_INITIAL_USER_COOKIES_FILE
fi

if [ -n "${WHIST_INITIAL_USER_BOOKMARKS_FILE+1}" ] && [ -f "$WHIST_INITIAL_USER_BOOKMARKS_FILE" ]; then
  rm $WHIST_INITIAL_USER_BOOKMARKS_FILE
fi

# Clean up traces of temporary files
unset WHIST_INITIAL_USER_COOKIES_FILE
unset WHIST_INITIAL_USER_BOOKMARKS_FILE

# Start the application that this mandelbox runs.
/usr/share/whist/run-as-whist-user.sh "/usr/bin/run-whist-application.sh" &
whist_application_runuser_pid=$!

echo "Whist application runuser pid: $whist_application_runuser_pid"

# Wait for run-whist-application.sh to write PID to file
until [ -f "$WHIST_APPLICATION_PID_FILE" ]
do
  sleep 0.1
done
whist_application_pid=$(cat $WHIST_APPLICATION_PID_FILE)
rm $WHIST_APPLICATION_PID_FILE

echo "Whist application pid: $whist_application_pid"

echo "Now sleeping until there are X clients..."

# Wait until the application has created its display before launching WhistServer.
#    This prevents a black no input window from appearing when a user connects.
until [ $(xlsclients -display :10 | wc -l) != 0 ]
do
  sleep 0.1
done

echo "Done sleeping until there are X clients..."
echo "done" > $WHIST_MAPPINGS_DIR/done_sleeping_until_X_clients
sync # Necessary so that even if the container exits very soon the host service sees the file written.

# Send in identifier
OPTIONS="$OPTIONS --identifier=$IDENTIFIER"

# The point of the named pipe redirection is so that $! will give us the PID of WhistServer, not of tee.
/usr/share/whist/WhistServer $OPTIONS &> >(tee $PROTOCOL_LOG_FILENAME) &
whist_server_pid=$!

# Wait for either whist-application or WhistServer to exit (both backgrounded processes).

# TODO: once our mandelboxes have bash 5.1 we will be able to deduce _which_
# application exited with the `-p` flag to `wait`.
wait -n
echo "Either WhistServer or whist-application exited with code $?"
echo "WhistServer PID: $whist_server_pid"
echo "runuser whist-application PID: $whist_application_runuser_pid"
echo "whist-application PID: $whist_application_pid"
echo "Remaining job PIDs: $(jobs -p)"

# Kill whatever is still running of WhistServer and whist-application, with SIGTERM.
kill $whist_application_pid ||:
kill $whist_server_pid ||:

# Wait for whist-application to finish terminating, ignoring exit code (since
wait $whist_application_runuser_pid ||:

echo "Both whist-application and WhistServer have exited."

# We now pass control over to `cleanup`, since we've reached the end of the script.
