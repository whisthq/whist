#!/usr/bin/env python3

import argparse
import os
import sys
import subprocess
import json

# TODO this is intended as a temporary program only to be used while
# developers are expected to interface directly with the staging/production
# databases instead of local copies. It should be removed when this is no
# longer the intended practice.

parser = argparse.ArgumentParser()
parser.add_argument(
    "env",
    help="which environment to pull configs for (either staging, production, or the name of an app)",
)
parser.add_argument(
    "--base-config",
    help="path to base config to override the remote config with, or None for no override."
    + " This file is expected to contain overrides such as pointing to a local Redis server.",
    default=os.path.join(
        os.path.dirname(os.path.realpath(__file__)), "dev-base-config.json"
    ),
)
args = parser.add_argument(
    "--out",
    help="where to write the env vars to, in addition to stdout",
    default=os.path.join(os.path.dirname(os.path.realpath(__file__)), ".env"),
)
args = parser.parse_args()

env_to_app_name = {
    "staging": "cube-celery-staging",
    "production": "cube-celery-vm",
    "reorganization": "cube-celery-staging4",
}
app_name = env_to_app_name.get(args.env, args.env)
heroku_proc = subprocess.run(
    ["heroku", "config", "--json", "--app", app_name],
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    shell=True,
)
if heroku_proc.returncode != 0:
    print(heroku_proc.stderr.decode("utf-8"))
    sys.exit(heroku_proc.returncode)
env_config = json.loads(heroku_proc.stdout.decode("utf-8"))

# The following variables of interest were determined by running the following
# command on `6c96c9e`:
# ```
# ag "getenv\(.+?\)" --only-matching --nogroup --nofilename | sort | uniq
# ```
useful_env_vars = [
    "ADMIN_PASSWORD",
    "AWS_ACCESS_KEY",
    "AWS_SECRET_KEY",
    "AZURE_CLIENT_ID",
    "AZURE_CLIENT_SECRET",
    "AZURE_SUBSCRIPTION_ID",
    "AZURE_TENANT_ID",
    # "CI",
    "DASHBOARD_PASSWORD",
    "DASHBOARD_USERNAME",
    "DATABASE_URL",
    "HEROKU_POSTGRESQL_ORANGE_URL",
    "ENDPOINT_SECRET",
    "FRONTEND_URL",
    "HOURLY_PLAN_ID",
    "JWT_SECRET_KEY",
    "MONTHLY_PLAN_ID",
    "PAPERTRAIL_PORT",
    "PAPERTRAIL_URL",
    "REDIS_URL",
    "SECRET_KEY",
    "SENDGRID_API_KEY",
    "SERVER_TYPE",
    "STRIPE_SECRET",
    # "TEST_HEROKU_PR_NUMBER",
    "UNLIMITED_PLAN_ID",
    "USE_PRODUCTION_DATABASE",
    "VM_GROUP",
    "VM_PASSWORD",
    "SMALLDISK_PLAN_ID",
    "MEDIUMDISK_PLAN_ID",
]

useful_config = {k: env_config.get(k) for k in useful_env_vars}
try:
    with open(args.base_config) as f:
        base_config = json.load(f)
except:
    base_config = {}
local_config = {**useful_config, **base_config}

# Follow the docker env file format: https://docs.docker.com/compose/env-file/
out_str = ""
for k, v in local_config.items():
    out_str += f"{k}={v}\n"
print(out_str)
with open(args.out, "w") as f:
    f.write(out_str)
print("Wrote %i env vars to %s" % (len(out_str.split("\n")), args.out))
