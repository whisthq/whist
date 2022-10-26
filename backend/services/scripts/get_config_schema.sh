#!/bin/bash
# get_config_schema.sh
#
# Download the configuration database's schema and write it out to both stdout
# and config_schema.sql. You must install and authenticate the Heroku CLI in
# order to run this script.

set -euo pipefail

POSTGRES_URL="$(heroku config:get HEROKU_POSTGRESQL_MAROON_URL -a whist-dev-scaling-service)"

docker run --rm postgres pg_dump --no-owner --no-privileges --schema-only --schema=public "$POSTGRES_URL" | sed '/CREATE SCHEMA public/s/./-- &/' | tee config_schema.sql
