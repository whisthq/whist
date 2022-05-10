#!/bin/bash

# Setup local database and Hasura servers for local scaling service testing.
# To run this script, the Heroku CLI must be installed and logged into your account.

# add_database_hasura will add the database given by the arguments
# to Hasura as a data source.
# Args: 
# $1: Hasura metadata URL, accesible locally
# $2: Database connection string, inside Docker network
add_database_hasura() {
curl -X POST $1 \
-H "Content-Type: application/json" \
--data-binary @- << EOF
{
  "type": "pg_add_source",
  "args": {
    "name": "Whist Local",
    "configuration": {
      "connection_info": {
        "database_url": "$2",
        "pool_settings": {
          "max_connections": 50,
          "idle_timeout": 180,
          "retries": 1,
          "pool_timeout": 360,
          "connection_lifetime": 600
        },
        "use_prepared_statements": true,
        "isolation_level": "read-committed"
      }
    },
    "replace_configuration": false
  }
}
EOF
}

# wait_database_ready tries to connect to the database
# using psql, or retries if it's not available.
# Args: 
# $1: Postgres connection string
wait_database_ready() {
cmds="\q"
while ! (psql "$1" <<< $cmds) &> /dev/null
do
  echo "Connection failed. Retrying in 2 seconds..."
  sleep 2
done
}

set -Eeuo pipefail

# Allow passing `--down` to spin down the docker-compose stack, instead of
# having to cd into this directory and manually run the command.
if [[ $* =~ [:space:]*--down[:space:]* ]]; then
  echo "Running \"docker-compose down\"."
  docker-compose down
  exit 0
fi

# PostgreSQL strings used for connecting to the databse on the host machine.
LOCAL_WHIST_PGSTRING="postgres://postgres:whistpass@localhost:5432/postgres"
LOCAL_CONFIG_PGSTRING="postgres://postgres:whistpass@localhost:9999/postgres"

# PostgreSQL strings used for connecting to the database inside the Docker network.
DOCKER_WHIST_PGSTRING="postgres://postgres:whistpass@postgres:5432/postgres"
DOCKER_CONFIG_PGSTRING="postgres://postgres:whistpass@postgres-config:5432/postgres"

# URLs to make calls to the Hasura API.
LOCAL_WHIST_URL="http://localhost:8080/v1/metadata"
LOCAL_CONFIG_URL="http://localhost:8082/v1/metadata"

# First pull the config schema because its not tracked in the codebase.
echo "Pulling config database schema..."
DEV_CONFIG_DATABASE=$(heroku config:get HEROKU_POSTGRESQL_MAROON_URL -a whist-dev-scaling-service)

# Start Hasura and Postgres databases.
docker-compose up -d

echo "Waiting for databases to be ready..."
wait_database_ready "$LOCAL_WHIST_PGSTRING" &
wait_database_ready "$LOCAL_CONFIG_PGSTRING" &
wait

echo "Setting up local databases..."
pg_dump --no-owner --no-privileges --schema-only "$DEV_CONFIG_DATABASE" > config_schema.sql
psql "$LOCAL_CONFIG_PGSTRING" < config_schema.sql
psql "$LOCAL_WHIST_PGSTRING" < ../../database/schema.sql

echo "Adding databases to Hasura servers..."
add_database_hasura "$LOCAL_WHIST_URL" "$DOCKER_WHIST_PGSTRING"
add_database_hasura "$LOCAL_CONFIG_URL" "$DOCKER_CONFIG_PGSTRING"

echo "Populating config database..."
psql "$LOCAL_CONFIG_PGSTRING" <<EOF
\x
INSERT INTO dev VALUES
    ('DESIRED_FREE_MANDELBOXES_US_EAST_1', '2');

INSERT INTO desktop_app_version(id, major, minor, micro, dev_rc, staging_rc, dev_commit_hash, staging_commit_hash, prod_commit_hash)
    VALUES
        (1, 2, 6, 15, 0, 0, 'dummy_commit_hash', 'dummy_commit_hash', 'dummy_commit_hash');
EOF

echo ""
echo "Cleaning up..."
rm config_schema.sql

green="\e[0;92m"
reset="\e[0m"

echo -e "${green}Your machine is ready to run the scaling service! You can run make run_scaling_service_localdevwithdb now to start developing."
echo "Make sure to visit http://localhost:8080 and http://localhost:8082 to track the tables/relationships in the Hasura console."
echo -e "In the future, simply run docker-compose up and the Hasura servers will be ready to use.${reset}"
