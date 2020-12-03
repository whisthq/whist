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
    "web")
	if [ -n "$HOT_RELOAD" ]; then
	    FLASK_ENV=development flask run --host "0.0.0.0" --port "$PORT"
	else
	    waitress-serve --port="$PORT" --url-scheme=https app:app
	fi ;;
    "celery")
	# The two containers share the same requirements.txt file, but we only
	# want the watchmedo utility to be installed in the Celery container.
	$([ -n "$HOT_RELOAD" ] && \
	      (pip install watchdog[watchmedo] >&2
	       echo "watchmedo auto-restart -R -d . --")) \
		   celery --app=celery_worker.celery_instance worker ;;
    *) echo "Specify either 'web' or 'celery' to determine what this instance will manifest as." ;;
esac
