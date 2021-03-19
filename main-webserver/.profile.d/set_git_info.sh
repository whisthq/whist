#!/bin/bash

echo "-----> Configuring git branch and commit hash to enviornment variables"
export BRANCH=$(git branch --show-current)
export COMMIT=$(git rev-parse --short HEAD)

echo $BRANCH
echo $COMMIT