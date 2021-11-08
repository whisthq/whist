#!/bin/bash

# This script is the first script run within the Whist mandelbox. It retrieves the
# mandelbox-specific parameters from Linux environment variables and sets them for
# the Whist mandelbox to use. It then starts systemd, which starts all of the
# Whist system services (.service files), starting Whist inside the mandelbox.

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

# If WHIST_INITIAL_USER_COOKIES is set, then create file
if [ -n "${WHIST_INITIAL_USER_COOKIES+1}" ]
then
  echo $WHIST_INITIAL_USER_COOKIES > /usr/share/fractal/private/user_cookies_env
fi

# If WHIST_COOKIE_UPLOAD_TARGET is set, then create file
if [ -n "${WHIST_COOKIE_UPLOAD_TARGET+1}" ]
then
  echo $WHIST_COOKIE_UPLOAD_TARGET > /usr/share/fractal/private/user_target_env
fi

unset WHIST_INITIAL_USER_COOKIES


# Remove a vestigal file that we do not use.
# This is how LXC used to read environment variables: see that deprecated code in
# https://github.com/moby/moby/blob/v1.9.1/daemon/execdriver/lxc/init.go#L107-L134
# We remove the file to make our infrastructure less obvious.
rm /.dockerenv

# Start systemd
exec /lib/systemd/systemd