#!/bin/bash

if [ ! -f ../../docker/.env ]; then
    echo "Make sure you have run retrieve_config.sh"
    exit 1
fi

# add to current env
export $(cat ../../docker/.env | xargs)

if [ ! -f db/db_schema.sql ]; then
    cd db
    bash fetch_db.sh
    cd ..
fi

# if not in CI, use docker-compose
if [ ! -z "${IN_CI}" ]; then
    docker-compose up -d --build
fi

# let db prepare
sleep 2

cd db
bash db_setup.sh

echo "Teardown with: docker-compose down"

