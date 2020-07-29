#!/bin/bash

response=$(curl -H "Authorization: token $GITHUB_PAT" -H "Accept: application/vnd.github.groot-preview+json" https://api.github.com/repos/fractalcomputers/main-webserver/commits/$HEROKU_TEST_RUN_COMMIT_VERSION/pulls)
