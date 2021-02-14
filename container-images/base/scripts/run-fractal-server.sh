#!/bin/bash

# This script starts the Fractal protocol server, and assumes that it is
# being run within a Fractal Docker container.

# Exit on errors and missing environment variables
set -Eeuo pipefail

# Set/Retrieve Container parameters 
FRACTAL_MAPPINGS_DIR=/fractal/resourceMappings
USER_CONFIGS_DIR=/fractal/userConfigs
IDENTIFIER_FILENAME=hostPort_for_my_32262_tcp
CONTAINER_ID_FILENAME=ContainerARN
PRIVATE_KEY_FILENAME=/usr/share/fractal/private/aes_key
WEBSERVER_URL_FILENAME=/usr/share/fractal/private/webserver_url
SENTRY_ENV_FILENAME=/usr/share/fractal/private/sentry_env

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
fi
OPTIONS="$OPTIONS --webserver=$WEBSERVER_URL"

# Send in Sentry environment, if set
if [ -f "$SENTRY_ENV_FILENAME" ]; then
    export SENTRY_ENV=$(cat $SENTRY_ENV_FILENAME)
    OPTIONS="$OPTIONS --environment=$SENTRY_ENV"
fi

# Create a google_drive folder in the user's home
ln -sf /fractal/cloudStorage/google_drive /home/fractal/

# This tar file, if it exists, has been retrieved from S3 and must be extracted
tarFile=$USER_CONFIGS_DIR/fractal-app-config.tar.gz
if [ -f "$tarFile" ]; then
    tar -xzf $tarFile -C $USER_CONFIGS_DIR
fi

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

# To assist the tar tool's "exclude" option, create a dummy tar file if it does not already exist
if [ ! -f "$tarFile" ]; then
    touch $tarFile
fi

# Create a .configready file that forces the display to wait until configs are synced
#     We are also forced to wait until the display has started
touch $USER_CONFIGS_DIR/.configready
until [ ! -f $USER_CONFIGS_DIR/.configready ]
do
    sleep 0.1
done

# Send in identifier
OPTIONS="$OPTIONS --identifier=$IDENTIFIER"

/usr/share/fractal/FractalServer $OPTIONS


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
    "logs": $(jq -Rs . </usr/share/fractal/log.txt)
}
END
)

# Poll for logs upload to finish
state=PENDING
while [[ $state =~ PENDING ]] || [[ $state =~ STARTED ]]; do
    # GET $WEBSERVER_URL/status/$LOGS_TASK_ID
    #   Get the status of the logs upload task.
    # JSON Response:
    #   state: The status for the task, "SUCCESS" on completion
    state=$(curl -L -X GET "$WEBSERVER_URL/status/$LOGS_TASK_ID" | jq -e ".state")

    # Since this is post-disconnect, we can afford to query with such a low frequency.
    # We choose to do this to reduce strain on the webserver, with the understanding
    # that we're adding at most 5 seconds to container shutdown time in expectation.
    sleep 5
done

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
