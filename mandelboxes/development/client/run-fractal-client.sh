#!/bin/bash

# This script starts the Whist protocol client

# Exit on subcommand errors
set -Eeuo pipefail

# FRACTAL_MAPPINGS_DIR=/fractal/resourceMappings
#PRIVATE_KEY_FILENAME=/usr/share/fractal/private/aes_key
#SERVER_IP_ADDRESS_FILENAME=/usr/share/fractal/private/server_ip
#SERVER_PORT_32262_FILENAME=/usr/share/fractal/private/port32262_filename
#SERVER_PORT_32263_FILENAME=/usr/share/fractal/private/port32263_filename
#SERVER_PORT_32273_FILENAME=/usr/share/fractal/private/port32273_filename

PROTOCOL_LOG_FILENAME=/usr/share/fractal/client.log

# Create list of command-line arguments to pass to the Whist protocol client
OPTIONS=""

# # Parse the server IP address, and add it to the options
# if [ -f "$SERVER_IP_ADDRESS_FILENAME" ]; then
#   export SERVER_IP_ADDRESS=$(cat $SERVER_IP_ADDRESS_FILENAME)
# else
#   export SERVER_IP_ADDRESS="127.0.0.1"
# fi
OPTIONS="$OPTIONS $SERVER_IP_ADDRESS"

# Parse the port 32262 on the server mandelbox
# if [ -f "$SERVER_PORT_32262_FILENAME" ]; then
#   export SERVER_PORT_32262=$(cat $SERVER_PORT_32262_FILENAME)
# else
#   export SERVER_PORT_32262="32262"
# fi
OPTIONS="$OPTIONS -p32262:$SERVER_PORT_32262"

# Parse the port 32263 on the server mandelbox
# if [ -f "$SERVER_PORT_32263_FILENAME" ]; then
#   export SERVER_PORT_32263=$(cat $SERVER_PORT_32263_FILENAME)
# else
#   export SERVER_PORT_32263="32263"
# fi
OPTIONS="$OPTIONS.32263:$SERVER_PORT_32263"

# Parse the port 32273 on the server mandelbox
# if [ -f "$SERVER_PORT_32273_FILENAME" ]; then
#   export SERVER_PORT_32273=$(cat $SERVER_PORT_32273_FILENAME)
# else
#   export SERVER_PORT_32273="32273"
# fi
OPTIONS="$OPTIONS.32273:$SERVER_PORT_32273"

# Send in AES private key, if set
# if [ -f "$PRIVATE_KEY_FILENAME" ]; then
#   export FRACTAL_AES_KEY=$(cat $PRIVATE_KEY_FILENAME)
#   OPTIONS="$OPTIONS -k=$FRACTAL_AES_KEY"
# fi
OPTIONS="$OPTIONS -k $FRACTAL_AES_KEY"

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

while true; do sleep 1000; done

# TODO: We now pass control over to `cleanup`, since we've reached the end of the script.
