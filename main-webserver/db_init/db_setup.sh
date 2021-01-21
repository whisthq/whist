#!/bin/bash

# make user. initially just "postgres" user exists
cmds="CREATE ROLE $POSTGRES_USER WITH LOGIN;\q;"
psql -h localhost -p 9999 -U postgres <<< $cmds

cmds="CREATE DATABASE $POSTGRES_DB;\q;"
psql -h localhost -p 9999 -U $POSTGRES_USER <<< $cmds

# copy schema
psql -h localhost -p 9999 -U $POSTGRES_USER -d $POSTGRES_DB -f db_create.sql
