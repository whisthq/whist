#!/bin/bash

# This script generates an ECS task definition JSON file for a specific application
# passed in as a parameter to the script (i.e. chrome).

# Warn on misconfigured arguments
if [ $# -eq 0 ] || [ $# -gt 1 ]; then
    echo "Invalid number of arguments"
    echo "This script takes in exactly 1 argument, the name of the application to generate a task definition for"
    exit 0
fi

# Currently, all apps have the same task definition parameters,
# so we only need to update the family tag in fractal-taskdef-template.json
#
# When/if there are app-specific parameters, we can update those
# using an if or switch block, or by storing diffs in JSON format.

app=$1

# Generate the task definition JSON for the app
# It's important to use fractal-taskdef-template here, else we get stdio issues
# rewriting fractal-base.json, from the base image, which we did before

cat fractal-taskdef-template.json | jq '.family |= "'$app'"' > $app.json

# Echo the task definition filename so the GitHub Actions CI workflow can get the app name
echo $app.json
