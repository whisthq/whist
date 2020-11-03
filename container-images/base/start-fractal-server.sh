#!/bin/sh

CONTAINER_ID=$(basename $(cat /proc/1/cpuset))
FRACTAL_MAPPINGS_DIR=/fractal/containerResourceMappings
IDENTIFIER_FILENAME=hostPort_for_my_32262_tcp
DPI_FILENAME=/usr/share/fractal/private/dpi
PRIVATE_KEY_FILENAME=/usr/share/fractal/private/aes_key
WEBSERVER_URL_FILENAME=/usr/share/fractal/private/webserver_url

IDENTIFIER=$(cat $FRACTAL_MAPPINGS_DIR/$CONTAINER_ID/$IDENTIFIER_FILENAME)

OPTIONS=""

# send in dpi if set
if [ -f "$DPI_FILENAME" ]; then
     export FRACTAL_DPI=$(cat $DPI_FILENAME)
     OPTIONS="$OPTIONS --dpi=$FRACTAL_DPI"
fi

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

/usr/share/fractal/FractalServer --identifier=$IDENTIFIER $FRACTAL_DPI_ARG $OPTIONS

