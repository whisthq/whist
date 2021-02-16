#!/bin/bash

# This script is the first script run within the Fractal container. It retrieves the
# container-specific parameters from Linux environment variables and sets them for
# the Fractal container to use. It then starts systemd, which starts all of the 
# Fractal system services (.service files), starting Fractal inside the container

# Exit on errors and missing environment variables
set -Eeuo pipefail

# If FRACTAL_AES_KEY is set, then create file
if [ -n "${FRACTAL_AES_KEY+1}" ]
then
    echo $FRACTAL_AES_KEY > /usr/share/fractal/private/aes_key
fi

# If WEBSERVER_URL is set, then create file
if [ -n "${WEBSERVER_URL+1}" ]
then
    echo $WEBSERVER_URL > /usr/share/fractal/private/webserver_url
fi

# If FRACTAL_DPI is set, then create file
if [ -n "${FRACTAL_DPI+1}" ]
then
    echo $FRACTAL_DPI > /usr/share/fractal/private/dpi
fi

# If SENTRY_ENV is set, then create file
if [ -n "${SENTRY_ENV+1}" ]
then
   echo $SENTRY_ENV > /usr/share/fractal/private/sentry_env
fi

# Unset the AWS key to make sure that this environment variable does not 
# leak in any way (probably redundant, but still good practice)
unset FRACTAL_AES_KEY

# Unset the DPI in case the webserver is still passing in this environment
# variable (that method has been superseded by a request to the host service)
unset FRACTAL_DPI

# Start systemd
exec /lib/systemd/systemd
