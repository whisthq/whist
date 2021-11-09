#!/bin/bash

# This script starts the Whist protocol client

# Exit on subcommand errors
set -Eeuo pipefail

# FRACTAL_MAPPINGS_DIR=/fractal/resourceMappings
PRIVATE_KEY_FILENAME=/usr/share/fractal/private/aes_key
PROTOCOL_LOG_FILENAME=/usr/share/fractal/client.log

# Create list of command-line arguments to pass to the Whist protocol client
OPTIONS=""

# Send in AES private key, if set
if [ -f "$PRIVATE_KEY_FILENAME" ]; then
  export FRACTAL_AES_KEY=$(cat $PRIVATE_KEY_FILENAME)
  OPTIONS="$OPTIONS -k=$FRACTAL_AES_KEY"
fi

# TODO: obtain all the necessary configs (port mappings, ip address)


# This function is called whenever the script exits, whether that is because we
# reach the end of this file (because either FractalServer or the Whist
# application died), or there was an error somewhere else in the script.
function cleanup {
  echo "cleanup() called! Shutting down the mandelbox..."

  # Make sure we shutdown the mandelbox when this script exits
  sudo shutdown now
}

# The point of the named pipe redirection is so that $! will give us the PID of FractalServer, not of tee.
/usr/share/fractal/FractalClient $OPTIONS > >(tee $PROTOCOL_LOG_FILENAME) &
fractal_client_pid=$!

# # Wait for either fractal-application or FractalServer to exit (both backgrounded processes).

# # TODO: once our mandelboxes have bash 5.1 we will be able to deduce _which_
# # application exited with the `-p` flag to `wait`.
wait -n
echo "FractalClient exited with code $?"
echo "FractalClient PID: $fractal_client_pid"
echo "Remaining job PIDs: $(jobs -p)"

# We now pass control over to `cleanup`, since we've reached the end of the script.
