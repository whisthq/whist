#! /bin/bash
docker run -d -p 8080:8080 \
	-e HASURA_GRAPHQL_DATABASE_URL=postgres://uap4ch2emueqo9:p4fd2c0800c4d0168f03f5576fba446d302fce35f0d65b6f47cb1846703a06dba@ec2-34-200-47-19.compute-1.amazonaws.com:5432/d9rf2k3vd6hvbm \
       -e HASURA_GRAPHQL_ENABLE_CONSOLE=true \
       -e HASURA_GRAPHQL_DEV_MODE=true \
	-e HASURA_GRAPHQL_ADMIN_SECRET=secret \
	-e HASURA_GRAPHQL_AUTH_HOOK=http://host.docker.internal:7730/hasura/auth \
       hasura/graphql-engine:v1.3.3
