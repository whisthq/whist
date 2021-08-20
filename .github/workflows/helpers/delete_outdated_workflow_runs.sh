#!/bin/bash

# Running this script will delete all workflow runs from outdated workflows no longer in `dev`
# on the fractal/fractal repository

# Exit on subcommand errors
set -Eeuo pipefail

user=fractal repo=fractal; gh api repos/$user/$repo/actions/runs \
--paginate -q '.workflow_runs[] | select(.head_branch != "dev") | "\(.id)"' | \
xargs -n1 -I % gh api repos/$user/$repo/actions/runs/% -X DELETE
