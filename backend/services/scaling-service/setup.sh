# !/bin/bash

# Setup local database and Hasura servers for local scaling service testing.
# To run this script, the Heroku CLI myst be installed and logged into your account.

# set -Eeuo pipefail

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

# PostgreSQL strings used for connecting to the databse on the host machine.
LOCAL_WHIST_PGSTRING="postgres://postgres:whistpass@localhost:5432/postgres"
LOCAL_CONFIG_PGSTRING="postgres://postgres:whistpass@localhost:9999/postgres"

# PostgreSQL strings used for connecting to the database inside the Docker network.
DOCKER_WHIST_PGSTRING="postgres://postgres:whistpass@postgres:5432/postgres"
DOCKER_CONFIG_PGSTRING="postgres://postgres:whistpass@postgres-config:9999/postgres"

# URLs to make calls to the Hasura API.
LOCAL_WHIST_URL="http://localhost:8080/v1/metadata"
LOCAL_CONFIG_URL="http://localhost:8082/v1/metadata"

# First pull the config schema because its not tracked in the codebase.
echo "Pulling config database schema..."
DEV_CONFIG_DATABASE=$(heroku config:get HEROKU_POSTGRESQL_MAROON_URL -a whist-dev-scaling-service)

# Start Hasura and Postgres databases.
docker-compose up -d

echo "Setting up local databases..."
pg_dump --no-owner --no-privileges --schema-only "$DEV_CONFIG_DATABASE" > config_schema.sql
psql "$LOCAL_CONFIG_PGSTRING" < config_schema.sql
psql "$LOCAL_WHIST_PGSTRING" < ../../database/schema.sql

echo "Adding databases to Hasura servers..."
add_database_hasura "$LOCAL_WHIST_URL" "$DOCKER_WHIST_PGSTRING"
add_database_hasura "$LOCAL_CONFIG_URL" "$DOCKER_CONFIG_PGSTRING"

# echo "Populating test config values..."
# psql "$LOCAL_WHIST_PGSTRING"
# psql <<EOF
#   \x
#   INSERT INTO dev(key, value)
#   VALUES
#       ("DESIRED_FREE_MANDELBOXES_US_EAST_1", "2")
#       ("", "")
#       ("", "")
#       ("", "")
# EOF


echo "Cleaning up..."
rm config_schema.sql

echo "Your machine is ready to run the scaling service! You can run make run_scaling_service_localdevwithdb now to start developing."
echo "Make sure to visit http://localhost:8080 and http://localhost:8082 to track the tables/relationships in the Hasura console."
