#!/bin/bash
# track.sh
#
# Track PostgresQL tables in Hasura.

set -euo pipefail

EXECUTABLE=${0##*/}

usage() {
    cat <<EOF >&2
Usage:
    $EXECUTABLE hostname table...
    -p PORT		 The port on which the Hasura console is being served
    table...		 The names of one or more PostgresQL tables
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
	-*)
	    case "$1" in
		-h)
		    usage
		    exit 0;;
		-p)
		    CONSOLE_PORT="$2"
		    if [ "$CONSOLE_PORT" = "" ]; then break; fi
		    shift 2;;
		*)
		    echo "Unknown argument '$1'" >&2
		    usage
		    exit 1;;
	    esac;;
	*)
	    if [ -v CONSOLE_HOSTNAME ]; then
		TABLES="${TABLES:+$TABLES }$1"
	    else
		CONSOLE_HOSTNAME="$1"
	    fi
	    shift 1;;
    esac
done

CONSOLE_PORT="${CONSOLE_PORT:-8080}"

for table in $TABLES; do
    curl -X POST "http://$CONSOLE_HOSTNAME:$CONSOLE_PORT/v1/metadata" \
	 -H "Content-Type: application/json" -H "X-Hasura-Role: admin" \
	 --data @- <<EOF
    {
	"type": "pg_track_table",
	"args": {
	    "table": "$table"
	}
    }
EOF
done
