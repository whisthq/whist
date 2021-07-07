#!/usr/bin/env python
#
# The exit codes for this script will mimic the migra diff tool
# that it depends on.
#
# migra uses the following exit codes:
# 0 is a successful run, producing no diff (identical schemas)
# 1 is a error
# 2 is a successful run, producing a diff (non-identical schemas)
# 3 is a successful run, but producing no diff, meaning the diff is "unsafe"
#
# We'll introduce our own exit codes to mark certain issues:
# 4 is a successful run, but producing a diff that does not result in
#   identical databases upon application.

import os
import sys
import helpers.messages as msg

sys.path.append(".github/workflows/helpers")

from resources import heroku_config
from helpers.utils import exec_sql, dump_schema, diff_schema, replace_postgres
from rich import print, traceback

traceback.install()

GITHUB_TOKEN = os.environ.get("GITHUB_TOKEN")
GITHUB_ISSUE = os.environ.get("GITHUB_ISSUE")
HEROKU_APP_NAME = os.environ.get("HEROKU_APP_NAME")
GITHUB_REPO = "fractal/fractal"
GITHUB_PR_URL = "https://github.com/fractal/fractal/pull/"
HEROKU_API_TOKEN = os.environ.get("HEROKU_DEVELOPER_API_KEY")
SERVER_DEV = "fractal-dev-server"
SERVER_STAGING = "fractal-staging-server"
SERVER_PROD = "fractal-prod-server"
PATH_BRANCH_SCHEMA = "main-webserver/db_migration/schema.sql"
COMMENT_IDENTIFIER = "AUTOMATED_DB_MIGRATION_MESSAGE"


def db_diff(sql_a, sql_b, db_a="postgresql:///a", db_b="postgresql:///b"):
    exec_sql(db_a, sql_a)
    exec_sql(db_b, sql_b)

    code, diff = diff_schema(db_a, db_b)

    if diff:
        exec_sql(db_a, diff)

        _, check_diff = diff_schema(db_a, db_b)

    if check_diff:
        return 4, check_diff
    return code, diff


if __name__ == "__main__":
    config = heroku_config(HEROKU_APP_NAME)

    db_url = replace_postgres(config["DATABASE_URL"])

    with open(PATH_BRANCH_SCHEMA, "r") as f:
        branch_schema = f.read()

    return_code, schema_diff = db_diff(dump_schema(db_url), branch_schema)

    if schema_diff:
        print(schema_diff)
    sys.exit(return_code)
