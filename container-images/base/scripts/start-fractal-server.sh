#!/bin/sh

# This script starts the Fractal protocol server, and assumes that it is
# being run within a Fractal Docker container.

# Set/Retrieve Container parameters 
CONTAINER_ID=$(basename $(cat /proc/1/cpuset))
FRACTAL_MAPPINGS_DIR=/fractal/containerResourceMappings
IDENTIFIER_FILENAME=hostPort_for_my_32262_tcp
PRIVATE_KEY_FILENAME=/usr/share/fractal/private/aes_key
WEBSERVER_URL_FILENAME=/usr/share/fractal/private/webserver_url
SENTRY_ENV_FILENAME=/usr/share/fractal/private/sentry_env

# Define a string-format identifier for this container
IDENTIFIER=$(cat $FRACTAL_MAPPINGS_DIR/$CONTAINER_ID/$IDENTIFIER_FILENAME)

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

# send in identifier
OPTIONS="$OPTIONS --identifier=$IDENTIFIER"

ln -sf /fractal/cloudStorage/$IDENTIFIER/google_drive /home/fractal/

/usr/share/fractal/FractalServer $OPTIONS

# Send logs to webserver
curl \
    --header "Content-Type: application/json" \
    --request POST \
    --data @- \
    $WEBSERVER_URL/logs \
    <<END
{
    "sender": "server",
    "identifier": "$IDENTIFIER",
    "secret_key": "$FRACTAL_AES_KEY",
    "logs": $(jq -Rs . </usr/share/fractal/log.txt)
}
END

