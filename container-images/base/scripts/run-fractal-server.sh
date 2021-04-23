#!/bin/bash

# This script starts the Fractal protocol server, and assumes that it is
# being run within a Fractal Docker container.

# Exit on subcommand errors
set -Eeuo pipefail

# Set/Retrieve Container parameters
FRACTAL_MAPPINGS_DIR=/fractal/resourceMappings
USER_CONFIGS_DIR=/fractal/userConfigs
IDENTIFIER_FILENAME=hostPort_for_my_32262_tcp
CONTAINER_ID_FILENAME=ContainerARN
PRIVATE_KEY_FILENAME=/usr/share/fractal/private/aes_key
WEBSERVER_URL_FILENAME=/usr/share/fractal/private/webserver_url
SENTRY_ENV_FILENAME=/usr/share/fractal/private/sentry_env
TIMEOUT_FILENAME=$FRACTAL_MAPPINGS_DIR/timeout

# Define a string-format identifier for this container
IDENTIFIER=$(cat $FRACTAL_MAPPINGS_DIR/$IDENTIFIER_FILENAME)

# Pull out the webserver identifier for this container
CONTAINER_ID=$(cat $FRACTAL_MAPPINGS_DIR/$CONTAINER_ID_FILENAME)

# Create list of command-line arguments to pass to the Fractal protocol server
OPTIONS=""

# Send in AES private key, if set
if [ -f "$PRIVATE_KEY_FILENAME" ]; then
    export FRACTAL_AES_KEY=$(cat $PRIVATE_KEY_FILENAME)
    OPTIONS="$OPTIONS --private-key=$FRACTAL_AES_KEY"
fi

# Send in Fractal webserver url, if set
if [ -f "$WEBSERVER_URL_FILENAME" ]; then
    export WEBSERVER_URL=$(cat $WEBSERVER_URL_FILENAME)
    OPTIONS="$OPTIONS --webserver=$WEBSERVER_URL"
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

# Create a google_drive folder in the user's home
ln -sf /fractal/cloudStorage/google_drive /home/fractal/

# While perhaps counterintuitive, "source" is the path in the userConfigs directory
#   and "destination" is the original location of the config file/folder.
#   This is because when creating symlinks, the userConfig path is the source
#   and the original location is the destination
# Iterate through the possible configuration locations and copy
for row in $(cat app-config-map.json | jq -rc '.[]'); do
    SOURCE_CONFIG_SUBPATH=$(echo ${row} | jq -r '.source')
    SOURCE_CONFIG_PATH=$USER_CONFIGS_DIR/$SOURCE_CONFIG_SUBPATH
    DEST_CONFIG_PATH=$(echo ${row} | jq -r '.destination')

    # If original config path does not exist, then continue
    if [ ! -f "$DEST_CONFIG_PATH" ] && [ ! -d "$DEST_CONFIG_PATH" ]; then
        continue
    fi

    # If the source path doesn't exist, then copy default configs to the synced app config folder
    if [ ! -f "$SOURCE_CONFIG_PATH" ] && [ ! -d "$SOURCE_CONFIG_PATH" ]; then
        cp -rT $DEST_CONFIG_PATH $SOURCE_CONFIG_PATH
    fi

    # Remove the original configs and symlink the new ones to the original locations
    rm -rf $DEST_CONFIG_PATH
    ln -sfnT $SOURCE_CONFIG_PATH $DEST_CONFIG_PATH
    chown -R fractal $SOURCE_CONFIG_PATH
done

# Delete broken symlinks from config
find $USER_CONFIGS_DIR -xtype l -delete

echo "Sleeping until .configready is gone..."

# Create a .configready file that forces the display to wait until configs are synced
#     We are also forced to wait until the display has started
touch $USER_CONFIGS_DIR/.configready
until [ ! -f $USER_CONFIGS_DIR/.configready ]
do
    sleep 0.1
done

echo "Done sleeping until .configready is gone..."
echo "Now sleeping until there are X clients..."

# Wait until the application has created its display before launching FractalServer.
#    This prevents a black no input window from appearing when a user connects.
until [ $(xlsclients -display :10 | wc -l) != 0 ]
do
    sleep 0.1
done

echo "Done sleeping until there are X clients..."

# Send in identifier
OPTIONS="$OPTIONS --identifier=$IDENTIFIER"

# Allow the command to fail without the script exiting, since we want to send logs/clean up the container.
if ! /usr/share/fractal/FractalServer $OPTIONS ; then
  echo "FractalServer exited with bad code $?"
else
  echo "FractalServer exited with good code 0"
fi

# If $WEBSERVER_URL is unset, then do not attempt shutdown requests.
if [[ ! ${WEBSERVER_URL+x} ]]; then
    exit 0
fi

# Find the correctly named log file
case ${SENTRY_ENV:-"dev"} in
    "production" )
        LOG_FILENAME="log.txt"
        ;;
    "staging" )
        LOG_FILENAME="log-staging.txt"
        ;;
    * )
        LOG_FILENAME="log-dev.txt"
        ;;
esac

# POST $WEBSERVER_URL/logs
#   Upload the logs from the protocol run to S3.
# JSON Parameters:
#   sender: "server" because these are server protocol logs
#   identifier: "$CONTAINER_ID" because this endpoint wants task ARN
#   secret_key: "$FRACTAL_AES_KEY" to verify that we are authorized to make this request
#   logs: Appropriately JSON-sanitize `log.txt`
# JSON Response:
#   ID: the ID for the task for uploading logs; we need this to finish before container delete
LOGS_TASK_ID=$(curl \
        --header "Content-Type: application/json" \
        --request POST \
        --data @- \
        $WEBSERVER_URL/logs \
        << END \
    | jq -er ".ID"
{
    "sender": "server",
    "identifier": "$CONTAINER_ID",
    "secret_key": "$FRACTAL_AES_KEY",
    "logs": $(jq -Rs . </usr/share/fractal/$LOG_FILENAME)
}
END
)

# After sending the logs request, but before we get deep into our polling loop,
# we should safely shut down the running application by properly stopping our
# X server. Note that systemd unit dependency relationships are such that
# After and Requires dependencies only apply to startup, and not to shutdown;
# hence, we can safely stop an ancestor service. This would not be the case
# if we were to use the PartOf relationship.
#
# Also note that this will have to be reworked if the service that calls
# this script starts doing so as a user instead of as root. The rework would
# involve adding a root service whose only job is to listen for a signal
# emitted here and to perform the same actions on receipt of the signal.
systemctl stop fractal-display

echo "We just ran 'systemctl stop fractal-display'"

get_task_state() {
    # GET $WEBSERVER_URL/status/$1
    #   Get the status of the provided task.
    # JSON Response:
    #   state: The status for the task, "SUCCESS" on completion
    curl -L -X GET "$WEBSERVER_URL/status/$1" | jq -e ".state"
}

echo "About to poll for log upload to finish..."

# Poll for logs upload to finish
state=$(get_task_state $LOGS_TASK_ID)
while [[ $state =~ PENDING ]] || [[ $state =~ STARTED ]]; do
    # Since this is post-disconnect, we can afford to query with such a low frequency.
    # We choose to do this to reduce strain on the webserver, with the understanding
    # that we're adding at most 5 seconds to container shutdown time in expectation.
    sleep 5

    state=$(get_task_state $LOGS_TASK_ID)
done

echo "Done polling for logs upload to finish. Sending a container/delete request to the webserver..."

# POST $WEBSERVER_URL/container/delete
#   Trigger the container deletion request in AWS.
# JSON Parameters:
#   container_id: The AWS container id for this container.
#   private_key: "$FRACTAL_AES_KEY" to verify that we are authorized to make this request
curl \
    --header "Content-Type: application/json" \
    --request POST \
    --data @- \
    $WEBSERVER_URL/container/delete \
    << END
{
    "container_id": "$CONTAINER_ID",
    "private_key":  "$FRACTAL_AES_KEY"
}
END

echo "Done sending a container/delete request to the websever. Shutting down the container..."

# Once the server has exited, we should just shutdown the container so it doesn't hang
sudo shutdown now
