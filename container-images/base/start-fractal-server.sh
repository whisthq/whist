#!/bin/sh

CONTAINER_ID=$(basename $(cat /proc/1/cpuset))
FRACTAL_MAPPINGS_DIR=/fractal/containerResourceMappings
IDENTIFIER_FILENAME=hostPort_for_my_32262_tcp
PRIVATE_KEY_FILENAME=/usr/share/fractal/private/aes_key

IDENTIFIER=$(cat $FRACTAL_MAPPINGS_DIR/$CONTAINER_ID/$IDENTIFIER_FILENAME)

# send in private key if set
if [ -f "$PRIVATE_KEY_FILENAME" ]
then
     export FRACTAL_AES_KEY=$(cat $PRIVATE_KEY_FILENAME)
     # use --private-key without argument to make it read the private key from env variable
     /usr/share/fractal/FractalServer --private-key=$FRACTAL_AES_KEY --identifier=$IDENTIFIER
else
     /usr/share/fractal/FractalServer --identifier=$IDENTIFIER
fi
