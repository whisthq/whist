#! /bin/bash

export $(cat ../docker/.env | xargs)

docker run -d -p 8080:8080 \
	-e HASURA_GRAPHQL_DATABASE_URL=$CONFIG_DB_URL \
       -e HASURA_GRAPHQL_ENABLE_CONSOLE=true \
       -e HASURA_GRAPHQL_DEV_MODE=true \
	-e HASURA_GRAPHQL_ADMIN_SECRET=secret \
	-e HASURA_GRAPHQL_AUTH_HOOK=http://host.docker.internal:7730/hasura/auth \
       hasura/graphql-engine:v1.3.3
