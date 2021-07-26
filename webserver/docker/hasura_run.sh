#!/bin/bash

# exit on error
set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# make sure current directory is `webserver/docker`
cd "$DIR"

export $(cat .env | xargs)

docker run -d -p "8080:8080" \
    -e HASURA_GRAPHQL_DATABASE_URL="$CONFIG_DB_URL" \
    -e HASURA_GRAPHQL_ENABLE_CONSOLE=true \
    -e HASURA_GRAPHQL_DEV_MODE=true \
    -e HASURA_GRAPHQL_ADMIN_SECRET=secret \
    "hasura/graphql-engine:v1.3.3"
