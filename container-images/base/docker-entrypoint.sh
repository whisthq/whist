#!/bin/bash

echo $FRACTAL_AES_KEY > /usr/share/fractal/private/aes_key
unset FRACTAL_AES_KEY
exec /lib/systemd/systemd