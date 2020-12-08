#!/bin/sh

CONTAINER_ID=$(basename $(cat /proc/1/cpuset))
FRACTAL_MAPPINGS_DIR=/fractal/containerResourceMappings
IDENTIFIER_FILENAME=hostPort_for_my_32262_tcp
DPI_FILENAME=DPI
PRIVATE_KEY_FILENAME=/usr/share/fractal/private/aes_key
WEBSERVER_URL_FILENAME=/usr/share/fractal/private/webserver_url

IDENTIFIER=$(cat $FRACTAL_MAPPINGS_DIR/$CONTAINER_ID/$IDENTIFIER_FILENAME)
FRACTAL_DPI=$(cat $FRACTAL_MAPPINGS_DIR/$CONTAINER_ID/$DPI_FILENAME)

OPTIONS=""

# send in private key if set
if [ -f "$PRIVATE_KEY_FILENAME" ]; then
    export FRACTAL_AES_KEY=$(cat $PRIVATE_KEY_FILENAME)
    OPTIONS="$OPTIONS --private-key=$FRACTAL_AES_KEY"
    # use --private-key without argument to make it read the private key from env variable
fi

# send in webserver url if set
if [ -f "$WEBSERVER_URL_FILENAME" ]; then
    export WEBSERVER_URL=$(cat $WEBSERVER_URL_FILENAME)
    OPTIONS="$OPTIONS --webserver=$WEBSERVER_URL"
fi

# mount cloud storage directories to home directory
# https://stackoverflow.com/questions/2107945/how-to-loop-over-directories-in-linux
# find "/fractal/cloudStorage/$IDENTIFIER" -maxdepth 1 -mindepth 1 -type d -exec ln -sf {} /home/fractal/


/usr/share/fractal/FractalServer --identifier=$IDENTIFIER $OPTIONS
