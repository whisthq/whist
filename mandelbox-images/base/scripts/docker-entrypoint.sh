#!/bin/bash

# This script is the first script run within the Fractal mandelbox. It retrieves the
# mandelbox-specific parameters from Linux environment variables and sets them for
# the Fractal mandelbox to use. It then starts systemd, which starts all of the
# Fractal system services (.service files), starting Fractal inside the mandelbox.

# Exit on subcommand errors
set -Eeuo pipefail

# If FRACTAL_AES_KEY is set, then create file
if [ -n "${FRACTAL_AES_KEY+1}" ]
then
    echo $FRACTAL_AES_KEY > /usr/share/fractal/private/aes_key
fi

# If SENTRY_ENV is set, then create file
if [ -n "${SENTRY_ENV+1}" ]
then
    echo $SENTRY_ENV > /usr/share/fractal/private/sentry_env
fi

# Unset the AWS key to make sure that this environment variable does not
# leak in any way (probably redundant, but still good practice)
unset FRACTAL_AES_KEY

# Remove a vestigal file that we do not use.
# This is how LXC used to read environment variables: see that deprecated code in
# https://github.com/moby/moby/blob/v1.9.1/daemon/execdriver/lxc/init.go#L107-L134
# We remove the file to make our infrastructure less obvious.
rm /.dockerenv

# Start systemd
exec /lib/systemd/systemd
