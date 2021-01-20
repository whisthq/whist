#!/bin/bash

# make database and user. right now just "postgres" user exists
cmds="CREATE DATABASE $POSTGRES_DB; CREATE ROLE $POSTGRES_USER WITH LOGIN; \q"
psql -h localhost -p 9999 -U postgres <<< $cmds

# copy schema
psql -h localhost -p 9999 -U $POSTGRES_USER -d $POSTGRES_DB -f db_create.sql
