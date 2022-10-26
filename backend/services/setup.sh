#!/bin/bash
# setup.sh
#
# TODO(owen): Write description

set -u

docker compose run setup

code=$?

docker compose down --remove-orphans

exit $code
