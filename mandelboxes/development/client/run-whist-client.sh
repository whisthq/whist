#!/bin/bash

# This script starts the Whist protocol client

# Exit on subcommand errors
set -Eeuo pipefail

#WHIST_MAPPINGS_DIR=/whist/resourceMappings
JSON_TRANSPORT_FILE=config.json
PROTOCOL_LOG_FILENAME=/usr/share/whist/client.log
SERVER_IP_ADDRESS=127.0.0.1
SERVER_PORT_32262=""
SERVER_PORT_32263=""
SERVER_PORT_32273=""
SERVER_AES_KEY=""

# Create list of command-line arguments to pass to the Whist protocol client
OPTIONS=""

#Sample JSON: {"dev_client_server_ip": "35.170.79.124", "dev_client_server_port_32262": 40618, "dev_client_server_port_32263": 31680, "dev_client_server_port_32273": 5923, "dev_client_server_aes_key": "70512c062ff1101f253be70e4cac81bc"}
WHIST_JSON_FILE=/whist/resourceMappings/config.json
if [[ -f $WHIST_JSON_FILE ]]; then
  if [ "$( jq 'has("dev_client_server_ip")' < $WHIST_JSON_FILE )" == "true"  ]; then
    SERVER_IP_ADDRESS="$(jq '.dev_client_server_ip' < $WHIST_JSON_FILE)"
    # Remove potential quotation marks
    SERVER_IP_ADDRESS=$(echo $SERVER_IP_ADDRESS | tr -d '"')
    # Add server IP address to options
    OPTIONS="$OPTIONS $SERVER_IP_ADDRESS"
  fi
  if [ "$( jq 'has("dev_client_server_port_32262")' < $WHIST_JSON_FILE )" == "true"  ]; then
    SERVER_PORT_32262="$(jq '.dev_client_server_port_32262' < $WHIST_JSON_FILE)"
    # Remove potential quotation marks
    SERVER_PORT_32262=$(echo $SERVER_PORT_32262 | tr -d '"')
    # Add server port 32262 address to options
    OPTIONS="$OPTIONS -p32262:$SERVER_PORT_32262"
  else
    echo "Server port 32262 not found in JSON data!"
    exit 1
  fi
  if [ "$( jq 'has("dev_client_server_port_32263")' < $WHIST_JSON_FILE )" == "true"  ]; then
    SERVER_PORT_32263="$(jq '.dev_client_server_port_32263' < $WHIST_JSON_FILE)"
    # Remove potential quotation marks
    SERVER_PORT_32263=$(echo $SERVER_PORT_32263 | tr -d '"')
    # Add server port 32263 address to options
    OPTIONS="$OPTIONS.32263:$SERVER_PORT_32263"
  else
    echo "Server port 32263 not found in JSON data!"
    exit 1
  fi
  if [ "$( jq 'has("dev_client_server_port_32273")' < $WHIST_JSON_FILE )" == "true"  ]; then
    SERVER_PORT_32273="$(jq '.dev_client_server_port_32273' < $WHIST_JSON_FILE)"
    # Remove potential quotation marks
    SERVER_PORT_32273=$(echo $SERVER_PORT_32273 | tr -d '"')
    # Add server port 32273 address to options
    OPTIONS="$OPTIONS.32273:$SERVER_PORT_32273"
  else
    echo "Server port 32273 not found in JSON data!"
    exit 1
  fi
  if [ "$( jq 'has("dev_client_server_aes_key")' < $WHIST_JSON_FILE )" == "true"  ]; then
    SERVER_AES_KEY="$(jq '.dev_client_server_aes_key' < $WHIST_JSON_FILE)"
    # Remove potential quotation marks
    SERVER_AES_KEY=$(echo $SERVER_AES_KEY | tr -d '"')
    # Add server AES key address to options
    OPTIONS="$OPTIONS -k $SERVER_AES_KEY"
  else
    echo "Server AES key not found in JSON data!"
    exit 1
  fi
fi


# The point of the named pipe redirection is so that $! will give us the PID of WhistServer, not of tee.
# Timeout will turn off the client once we are done gathering metrics data. This value here should match the one in the .github/workflows/helpers/aws/streaming_performance_tester.py file
timeout 240s /usr/share/whist/WhistClient $OPTIONS > >(tee $PROTOCOL_LOG_FILENAME) &
whist_client_pid=$!

# TODO: once our mandelboxes have bash 5.1 we will be able to deduce _which_
# application exited with the `-p` flag to `wait`.
wait -n
echo "WhistClient exited with code $?"
echo "WhistClient PID: $whist_client_pid"


