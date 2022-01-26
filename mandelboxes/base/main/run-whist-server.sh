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

### BEGIN USER CONFIG RETRIEVE ###

# Begin wait loop to get userConfigs
WHIST_MAPPINGS_DIR=/whist/resourceMappings
USER_CONFIGS_DIR=/whist/userConfigs
APP_CONFIG_MAP_FILENAME=/usr/share/whist/app-config-map.json

block-until-file-exists.sh $WHIST_MAPPINGS_DIR/.configReady >&1

# Symlink loaded user configs into the appropriate folders

# While perhaps counterintuitive, "source" is the path in the userConfigs directory
#   and "destination" is the original location of the config file/folder.
#   This is because when creating symlinks, the userConfig path is the source
#   and the original location is the destination
# Iterate through the possible configuration locations and copy
for row in $(jq -rc '.[]' < $APP_CONFIG_MAP_FILENAME); do
  SOURCE_CONFIG_SUBPATH=$(echo "${row}" | jq -rc '.source')
  SOURCE_CONFIG_PATH=$USER_CONFIGS_DIR/$SOURCE_CONFIG_SUBPATH
  DEST_CONFIG_PATH=$(echo "${row}" | jq -rc '.destination')

  # If original config path does not exist, then continue
  if [ ! -f "$DEST_CONFIG_PATH" ] && [ ! -d "$DEST_CONFIG_PATH" ]; then
    continue
  fi

  # If the source path doesn't exist, then copy default configs to the synced app config folder
  if [ ! -f "$SOURCE_CONFIG_PATH" ] && [ ! -d "$SOURCE_CONFIG_PATH" ]; then
    cp -rT "$DEST_CONFIG_PATH" "$SOURCE_CONFIG_PATH"
  fi

  # Remove the original configs and symlink the new ones to the original locations
  rm -rf "$DEST_CONFIG_PATH"
  ln -sfnT "$SOURCE_CONFIG_PATH" "$DEST_CONFIG_PATH"
  chown -R whist "$SOURCE_CONFIG_PATH"
done

# Delete broken symlinks from config
find $USER_CONFIGS_DIR -xtype l -delete

### END USER CONFIG RETRIEVE ###

# Set/Retrieve Mandelbox parameters
WHIST_MAPPINGS_DIR=/whist/resourceMappings
IDENTIFIER_FILENAME=hostPort_for_my_32262_tcp
PRIVATE_KEY_FILENAME=$WHIST_PRIVATE_DIR/aes_key
BROWSER_DATA_FILE_FILENAME=$WHIST_PRIVATE_DIR/user_browser_data_file
USER_DEST_BROWSER_FILENAME=$WHIST_PRIVATE_DIR/user_dest_browser
TIMEOUT_FILENAME=$WHIST_MAPPINGS_DIR/timeout
WHIST_APPLICATION_PID_FILE=/home/whist/whist-application-pid
PROTOCOL_LOG_FILENAME=/usr/share/whist/server.log
TELEPORT_LOG_FILENAME=/usr/share/whist/teleport-drag-drop.log
WHIST_JSON_FILE=/whist/resourceMappings/config.json

# Parse options from JSON transport file
LOCAL_CLIENT=false # true if the client app was `yarn start` or `package:local`
if [[ -f $WHIST_JSON_FILE ]]; then
  if [ "$( jq -rc 'has("local_client")' < $WHIST_JSON_FILE )" == "true"  ]; then
    LOCAL_CLIENT="$(jq -rc '.local_client' < $WHIST_JSON_FILE)"
  fi
fi

# Define a string-format identifier for this mandelbox
IDENTIFIER=$(cat $WHIST_MAPPINGS_DIR/$IDENTIFIER_FILENAME)

# Create list of command-line arguments to pass to the Whist protocol server
OPTIONS=""

# Send in AES private key, if set
if [ -f "$PRIVATE_KEY_FILENAME" ]; then
  WHIST_AES_KEY=$(cat $PRIVATE_KEY_FILENAME)
  export WHIST_AES_KEY
  OPTIONS="$OPTIONS --private-key=$WHIST_AES_KEY"
fi

# Send in Sentry environment, if set, except for the LOCAL_CLIENT case,
# since local clients might be in a version mismatch with server protocol
if [ -f "$SENTRY_ENV_FILENAME" ] && [ "$LOCAL_CLIENT" == "false" ]; then
  SENTRY_ENV=$(cat $SENTRY_ENV_FILENAME)
  export SENTRY_ENV
  OPTIONS="$OPTIONS --environment=$SENTRY_ENV"
fi

# Send in timeout, if set
if [ -f "$TIMEOUT_FILENAME" ]; then
  TIMEOUT=$(cat $TIMEOUT_FILENAME)
  export TIMEOUT
  OPTIONS="$OPTIONS --timeout=$TIMEOUT"
fi

# We use named pipe redirection for consistency with our WhistServer launch setup
# &> redirects both stdout and stdin together; shorthand for '> XYZ 2>&1'
/usr/share/whist/run-as-whist-user.sh "/usr/bin/run-whist-teleport-drag-drop.sh" &> >(tee $TELEPORT_LOG_FILENAME) &

# This function is called whenever the script exits, whether that is because we
# reach the end of this file (because either WhistServer or the Whist
# application died), or there was an error somewhere else in the script.
function cleanup {
  echo "cleanup() called! Shutting down the mandelbox..."

  # Make sure we shutdown the mandelbox when this script exits
  sudo shutdown now
}

ENV_NAME=$(cat $SENTRY_ENV_FILENAME)
export ENV_NAME
if [ "$ENV_NAME" != "localdev" ]; then
  # Make sure `cleanup` gets called on script exit in all environments except localdev.
  trap cleanup EXIT ERR
fi

# Set user upload target, if file exists
if [ -f "$USER_DEST_BROWSER_FILENAME" ] && [ -f "$BROWSER_DATA_FILE_FILENAME" ]; then
  # Imports user browser data if file exists
  python3 /usr/share/whist/import_user_browser_data.py "$(cat $USER_DEST_BROWSER_FILENAME)" "$(cat $BROWSER_DATA_FILE_FILENAME)"

  # Remove temporary files
  rm -f "$(cat $BROWSER_DATA_FILE_FILENAME)"
  rm $BROWSER_DATA_FILE_FILENAME
  rm $USER_DEST_BROWSER_FILENAME
fi

# Start the application that this mandelbox runs.
/usr/share/whist/run-as-whist-user.sh "/usr/bin/run-whist-application.sh" &
whist_application_runuser_pid=$!

echo "Whist application runuser pid: $whist_application_runuser_pid"

# Wait for run-whist-application.sh to write PID to file
block-until-file-exists.sh $WHIST_APPLICATION_PID_FILE >&1
whist_application_pid=$(cat $WHIST_APPLICATION_PID_FILE)
rm $WHIST_APPLICATION_PID_FILE

echo "Whist application pid: $whist_application_pid"

echo "Now sleeping until there are X clients..."

# Wait until the application has created its display before launching WhistServer.
# This prevents a black no input window from appearing when a user connects.
until [ "$(xlsclients -display :10 | wc -l)" != 0 ]
do
  sleep 0.1
done

echo "Done sleeping until there are X clients..."
echo "done" > $WHIST_MAPPINGS_DIR/done_sleeping_until_X_clients
sync # Necessary so that even if the container exits very soon the host service sees the file written.

# Send in identifier
OPTIONS="$OPTIONS --identifier=$IDENTIFIER"

# The point of the named pipe redirection is so that $! will give us the PID of WhistServer, not of tee.
/usr/share/whist/WhistServer "$OPTIONS" &> >(tee $PROTOCOL_LOG_FILENAME) &
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
kill "$whist_application_pid" ||:
kill $whist_server_pid ||:

# Wait for whist-application to finish terminating, ignoring exit code (since
wait $whist_application_runuser_pid ||:

echo "Both whist-application and WhistServer have exited."

# We now pass control over to `cleanup`, since we've reached the end of the script.
