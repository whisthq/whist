#!/bin/bash

# This script generates an ECS task definition JSON file for a specific application
# passed in as a parameter to the script (i.e. chrome).
# Arguments: $1 - app name (e.g. fractal-browsers-chrome) $2 - deploy environment (prod, staging, or dev)
#   $3 - Sentry environment (prod, staging, or none)

# Warn on misconfigured arguments
if [ $# -eq 0 ] || [ $# -gt 1 ]; then
    echo "Invalid number of arguments"
    echo "This script takes in exactly 1 argument, the name of the application to generate a task definition for"
    exit 0
fi

# Currently, all apps have the same task definition parameters,
# so we only need to update the family and Sentry environment tags (prod, staging, or empty string)
# in fractal-taskdef-template.json
#
# When/if there are app-specific parameters, we can update those
# using an if or switch block, or by storing diffs in JSON format.

app=$1
deploy_env=$2
sentry_env=$3

json_filepath=$deploy_env-$1.json # e.g. prod-fractal-browsers-chrome.json

# Generate the task definition JSON for the app
# It's important to use fractal-taskdef-template here, else we get stdio issues
# rewriting fractal-base.json, from the base image, which we did before

cat fractal-taskdef-template.json | jq '.family |= "'$app'"' | jq '.environment[2] |= "'$sentry_env'"' > $json_filepath

# Echo the task definition filename so the GitHub Actions CI workflow can get the app name
echo $json_filepath
