#!/bin/bash

# ${1} Number of AWS lambda functions to run

# exit on error and missing env var
set -Eeuo pipefail

NUM_CALLS=$1
SAVE_FOLDER="aws_lambda_dump"

rm -rf $SAVE_FOLDER
mkdir $SAVE_FOLDER

NUM_PROCS=2
NUM_REQS=2

echo "Invoking $NUM_CALLS calls to load test..."
for ((i=1; i<=$NUM_CALLS; i++));
do
	# if this goes wrong, there will be no message about it
	(nohup aws lambda invoke --function-name LoadTest --payload \
		"{\"NUM_PROCS\": ${NUM_PROCS}, \
		\"NUM_REQS\": ${NUM_REQS}, \
		\"WEB_URL\": \"${WEB_URL}\", \
		\"ADMIN_TOKEN\": \"${ADMIN_TOKEN}\", \
		\"FUNC_ID\": ${i}}" \
		--cli-binary-format raw-in-base64-out \
		${SAVE_FOLDER}/out_${i}.json &) \
		&> /dev/null
done

# wait for file to be made
while [ ! -f "${SAVE_FOLDER}/out_${NUM_CALLS}.json" ]; do
	echo "Waiting for load tests to finish..."
    sleep 5
done

