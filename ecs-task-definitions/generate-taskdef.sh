#!/bin/bash

# Currently, all apps have the same task definition parameters,
# so we only need to update the family tag.
#
# When there are app-specific parameters, we can update those
# using an if or switch block, or by storing diffs in json format

app=$1 # TODO: warn on misconfigured args

# it's important to use fractal-template here, else we get stdio issues
# rewriting fractal-base.json, from the base image, which we did before
cat fractal-template.json | jq '.family |= "'$app'"' > $app.json
echo $app.json # we echo so that the workflow can easily get the app name
