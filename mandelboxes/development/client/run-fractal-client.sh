#!/bin/bash

# This script starts the Whist protocol client

# Exit on subcommand errors
set -Eeuo pipefail

#FRACTAL_MAPPINGS_DIR=/fractal/resourceMappings
JSON_TRANSPORT_FILE=config.json
PROTOCOL_LOG_FILENAME=/usr/share/fractal/client.log
SERVER_IP_ADDRESS=127.0.0.1
SERVER_PORT_32262=""
SERVER_PORT_32263=""
SERVER_PORT_32273=""
SERVER_AES_KEY=""

# Create list of command-line arguments to pass to the Whist protocol client
OPTIONS=""

#Sample JSON: {"perf_client_server_ip": "35.170.79.124", "perf_client_server_port_32262": 40618, "perf_client_server_port_32263": 31680, "perf_client_server_port_32273": 5923, "perf_client_server_aes_key": "70512c062ff1101f253be70e4cac81bc"}
FRACTAL_JSON_FILE=/fractal/resourceMappings/config.json
if [[ -f $FRACTAL_JSON_FILE ]]; then
  if [ "$( jq 'has("perf_client_server_ip")' < $FRACTAL_JSON_FILE )" == "true"  ]; then
    SERVER_IP_ADDRESS="$(jq '.perf_client_server_ip' < $FRACTAL_JSON_FILE)"
    # Remove potential quotation marks
    SERVER_IP_ADDRESS=$(echo $SERVER_IP_ADDRESS | tr -d '"')
    # Add server IP address to options
    OPTIONS="$OPTIONS $SERVER_IP_ADDRESS"
  fi
  if [ "$( jq 'has("perf_client_server_port_32262")' < $FRACTAL_JSON_FILE )" == "true"  ]; then
    SERVER_PORT_32262="$(jq '.perf_client_server_port_32262' < $FRACTAL_JSON_FILE)"
    # Remove potential quotation marks
    SERVER_PORT_32262=$(echo $SERVER_PORT_32262 | tr -d '"')
    # Add server port 32262 address to options
    OPTIONS="$OPTIONS -p32262:$SERVER_PORT_32262"
  else
    echo "Server port 32262 not found in JSON data!"
    exit 1
  fi
  if [ "$( jq 'has("perf_client_server_port_32263")' < $FRACTAL_JSON_FILE )" == "true"  ]; then
    SERVER_PORT_32263="$(jq '.perf_client_server_port_32263' < $FRACTAL_JSON_FILE)"
    # Remove potential quotation marks
    SERVER_PORT_32263=$(echo $SERVER_PORT_32263 | tr -d '"')
    # Add server port 32263 address to options
    OPTIONS="$OPTIONS.32263:$SERVER_PORT_32263"
  else
    echo "Server port 32263 not found in JSON data!"
    exit 1
  fi
  if [ "$( jq 'has("perf_client_server_port_32273")' < $FRACTAL_JSON_FILE )" == "true"  ]; then
    SERVER_PORT_32273="$(jq '.perf_client_server_port_32273' < $FRACTAL_JSON_FILE)"
    # Remove potential quotation marks
    SERVER_PORT_32273=$(echo $SERVER_PORT_32273 | tr -d '"')
    # Add server port 32273 address to options
    OPTIONS="$OPTIONS.32273:$SERVER_PORT_32273"
  else
    echo "Server port 32273 not found in JSON data!"
    exit 1
  fi
  if [ "$( jq 'has("perf_client_server_aes_key")' < $FRACTAL_JSON_FILE )" == "true"  ]; then
    SERVER_AES_KEY="$(jq '.perf_client_server_aes_key' < $FRACTAL_JSON_FILE)"
    # Remove potential quotation marks
    SERVER_AES_KEY=$(echo $SERVER_AES_KEY | tr -d '"')
    # Add server AES key address to options
    OPTIONS="$OPTIONS -k $SERVER_AES_KEY"
  else
    echo "Server AES key not found in JSON data!"
    exit 1
  fi
fi


# The point of the named pipe redirection is so that $! will give us the PID of FractalServer, not of tee.
# Timeout will turn off the client once we are done gathering perf data. This value here should match the one in the .github/workflows/helpers/aws/streaming_performance_tester.py file
timeout 240s /usr/share/fractal/FractalClient $OPTIONS > >(tee $PROTOCOL_LOG_FILENAME) &
fractal_client_pid=$!

# TODO: once our mandelboxes have bash 5.1 we will be able to deduce _which_
# application exited with the `-p` flag to `wait`.
wait -n
echo "FractalClient exited with code $?"
echo "FractalClient PID: $fractal_client_pid"


