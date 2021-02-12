#!/usr/bin/env python

import os
import pretty_errors
import containers
import heroku
import postgres
from pprint import pprint
from pathlib import Path
import config



print(containers.remove_all_containers())

# Create container for DB-MERGING
#
# container_merging = containers.run_postgres_container(config.DB_MERGING)

# print(containers.all_containers())

# while True:
#     print(container_merging.status)
# Write db.sql to DB-MERGING
# conn_port_A = DB_MERGING["ports"]["localhost"]
# postgres.sql_commands(conn_port_A, SCHEMA_MERGING_PATH)
# Create container for DB-B
# pg_dump > dump.sql on LIVE-DB
# Write dump.sql to DB-B
# Run migra > diff.sql on DB-A and DB-B
# Run migra > test.sql on DB-B
# If test.sql is empty,
#     return diff.sql
#     else ERROR
