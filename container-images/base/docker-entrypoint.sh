#!/bin/bash

# if FRACTAL_AES_KEY is set, then create file and TODO: unset
if [ -n "${FRACTAL_AES_KEY+1}" ]
then
    echo $FRACTAL_AES_KEY > /usr/share/fractal/private/aes_key
fi
exec /lib/systemd/systemd