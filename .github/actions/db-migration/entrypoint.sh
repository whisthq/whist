#!/bin/bash

set -Eeuo pipefail

# We're installing postgres-13 in our docker file. The version in the
# command below needs to match.

# This command starts a postgres server, creates two databases, and then
# runs the arguments to this script as a process.

# Anything saved into a STATE_ variable will be accessible from the
# post-entrypoint.sh script, which in GitHub Actions will run after
# this one.

set +e # allow any exit-code; we will handle return codes directly
STATE_DIFF=$(pg_ctlcluster 13 main start \
                       && psql --quiet --command "create database a;" \
                       && psql --quiet --command "create database b;" \
                       && "$@") # passing in the actual process as args
STATE_CODE=$?
set -e # undo allowing any exit-code

echo "$STATE_DIFF" #

# GitHub Actions requires a 0 exit code, or it will crash.
# We accept 2, 3, and 4 exit codes from our diff program, as they
# denominate diff states. We must check for those, and return 0
# if we find any.
case $STATE_CODE in
    2)
        exit 0
    ;;
    3)
        exit 0
    ;;
    4)
        exit 0
    ;;
    *)
        exit $STATE_CODE
esac
