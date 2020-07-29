#!/bin/bash

export TEST_HEROKU_PR_NUMBER=$(echo $response | python -c "import json,sys;obj=json.load(sys.stdin);print(obj[0]['number']);")
echo $TEST_HEROKU_PR_NUMBER
