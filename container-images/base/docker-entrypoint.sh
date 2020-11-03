#!/bin/bash

# if FRACTAL_AES_KEY is set, then create file
if [ -n "${FRACTAL_AES_KEY+1}" ]
then
    echo $FRACTAL_AES_KEY > /usr/share/fractal/private/aes_key
fi

# if WEBSERVER_URL is set, then create file
if [ -n "${WEBSERVER_URL+1}" ]
then
    echo $WEBSERVER_URL > /usr/share/fractal/private/webserver_url
fi

# make sure this environment variable does not leak in any way (probably
# redundant, but still good practice)
unset FRACTAL_AES_KEY
exec /lib/systemd/systemd
