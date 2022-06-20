#!/bin/bash

# This script starts the Whist protocol client (only runs on Linux)

# Exit on subcommand errors
set -Eeuo pipefail

#WHIST_MAPPINGS_DIR=/whist/resourceMappings
PROTOCOL_LOG_FILENAME=/usr/share/whist/client.log
SERVER_IP_ADDRESS=127.0.0.1
SERVER_PORT_MAPPINGS=""
SERVER_AES_KEY=""
INITIAL_URLS=""

# Sample JSON: {"dev_client_server_ip": "35.170.79.124", "dev_client_server_port_mappings": "32262:19020.32263:17242.32273:29843", "dev_client_server_aes_key": "70512c062ff1101f253be70e4cac81bc"}
WHIST_JSON_FILE=/whist/resourceMappings/config.json
if [[ -f $WHIST_JSON_FILE ]]; then
  if [ "$( jq -rc 'has("dev_client_server_ip")' < $WHIST_JSON_FILE )" == "true"  ]; then
    SERVER_IP_ADDRESS="$(jq -rc '.dev_client_server_ip' < $WHIST_JSON_FILE)"
  fi
  if [ "$( jq -rc 'has("dev_client_server_port_mappings")' < $WHIST_JSON_FILE )" == "true"  ]; then
    SERVER_PORT_MAPPINGS="$(jq -rc '.dev_client_server_port_mappings' < $WHIST_JSON_FILE)"
  else
    echo "Server port mappings not found in JSON data!"
    exit 1
  fi
  if [ "$( jq -rc 'has("dev_client_server_aes_key")' < $WHIST_JSON_FILE )" == "true"  ]; then
    SERVER_AES_KEY="$(jq -rc '.dev_client_server_aes_key' < $WHIST_JSON_FILE)"
  else
    echo "Server AES key not found in JSON data!"
    exit 1
  fi
  if [ "$( jq -rc 'has("initial_urls")' < $WHIST_JSON_FILE )" == "true"  ]; then
    INITIAL_URLS="$(jq -rc '.initial_urls' < $WHIST_JSON_FILE)"
  else
    echo "Initial URL(s) not found in JSON data!"
  fi
fi

# Use pipe to pass arguments to the protocol to be able to open multiple URLs
(
  cat <<EOF
ports?$SERVER_PORT_MAPPINGS
private-key?$SERVER_AES_KEY
server-ip?$SERVER_IP_ADDRESS
open-url?$INITIAL_URLS
finished
EOF
) | timeout 240s /usr/share/whist/WhistClient --dynamic-arguments &> >(tee $PROTOCOL_LOG_FILENAME) &
# The point of the named pipe redirection is so that $! will give us the PID of WhistServer, not of tee.
# Timeout will turn off the client once we are done gathering metrics data. This value here should match the one in the protocol/test/streaming_e2e_tester.py file
whist_client_pid=$!

# TODO: once our mandelboxes have bash 5.1 we will be able to deduce _which_
# application exited with the `-p` flag to `wait`.
wait -n
echo "WhistClient exited with code $?"
echo "WhistClient PID: $whist_client_pid"
