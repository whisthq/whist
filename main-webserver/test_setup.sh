#!/bin/bash

echo $GITHUB_PAT
echo $HEROKU_TEST_RUN_COMMIT_VERSION

response=$(curl -H "Authorization: token $GITHUB_PAT" -H "Accept: application/vnd.github.groot-preview+json" https://api.github.com/repos/fractalcomputers/main-webserver/commits/$HEROKU_TEST_RUN_COMMIT_VERSION/pulls)
echo $response
export TEST_HEROKU_PR_NUMBER=$(echo $response | python -c "import json,sys;obj=json.load(sys.stdin);print(obj[0]['number']);")
echo $TEST_HEROKU_PR_NUMBER
