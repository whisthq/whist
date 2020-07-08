#!/bin/bash

# Used as the entrypoint by Dockerfile to determine if the instance
# should be running as `web` or `celery`.

# TODO use this as the entrypoint for the Procfile as well, e.g.
# Procfile
#   web: ./stem-cell.sh web
#   celery: ./stem-cell.sh celery
#
# This will ensure increased uniformity between dev and production
# environments.

case "$1" in
    "web") exec waitress-serve --port="$PORT" app:app ;;
    "celery") exec celery worker --app=celery_worker.celery ;;
    *) echo "Specify either 'web' or 'celery' to determine what this instance will manifest as." ;;
esac