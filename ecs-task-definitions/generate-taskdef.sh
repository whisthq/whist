#!/bin/bash

# This script generates an ECS task definition JSON file for a specific application
# passed in as a parameter to the script (i.e. chrome).
# Arguments: $1 - app name (e.g. fractal-prod-browsers-chrome) $2 - Sentry environment (prod, staging, or none)

# Exit on subcommand errors
set -Eeuo pipefail

# Warn on misconfigured arguments
if [ $# -eq 0 ] || [ $# -gt 2 ]; then
    echo "Invalid number of arguments"
    echo "This script takes in up to 2 arguments, first the name of the application to generate a task definition for"
    echo "and second the Sentry environment, if provided"
    exit 0
fi

# Currently, all apps have the same task definition parameters,
# so we only need to update the family and Sentry environment tags (prod, staging, or empty string)
# in fractal-taskdef-template.json
#
# When/if there are app-specific parameters, we can update those
# using an if or switch block, or by storing diffs in JSON format.

# in the workflow, $1 will be passed in with the deploy environment in the name (e.g. fractal-prod-browsers-chrome)
app=$1
sentry_env="${2:-}"

# Generate the task definition JSON for the app
# It's important to use fractal-taskdef-template here, else we get stdio issues
# rewriting fractal-base.json, from the base image, which we did before
# The following sets the "family" to the app name and the SENTRY_ENV env var to $sentry_env

cat fractal-taskdef-template.json | jq '.family |= "'$app'"' | jq -r '{"name":"SENTRY_ENV", "value":"'$sentry_env'"} as $v | (.containerDefinitions[].environment[] | select(.name==$v.name))|=$v' > $app.json

# Echo the task definition filename so the GitHub Actions CI workflow can get the app name
echo $app.json
