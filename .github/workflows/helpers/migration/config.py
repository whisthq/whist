#!/usr/bin/env python
from pathlib import Path


TIMEOUT_SECONDS = 10

FRACTAL_ROOT = Path.cwd().parent.parent

HEROKU_BASE_URL = "https://api.heroku.com"

HEROKU_APP_NAME = "fractal-dev-server"

SCHEMA_MERGING_NAME = "db.sql"

SCHEMA_MERGING_PATH = FRACTAL_ROOT.joinpath(SCHEMA_MERGING_NAME)

DB_MERGING = {"ports": {"localhost": 9900,
                        "container": 5432}}

DB_CURRENT = {"ports": {"localhost": 9995,
                        "container": 5432}}
