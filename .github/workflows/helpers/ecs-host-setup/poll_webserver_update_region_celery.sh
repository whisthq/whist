#!/bin/bash

# Arguments:
# ${1} the webserver url
# ${2} the task ID in question
# ${3} the admin token to use to get full logs

# Exit on errors and missing environment variables
set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# make sure current directory is `.github/workflows/helpers/ecs-host-setup/`
cd "$DIR"

# poll for task to finish
state=PENDING
echo "ID: $2"
while [[ $state =~ PENDING ]] || [[ $state =~ STARTED ]]; do
    status=$(curl -L -X GET -H "Authorization: Bearer ${3}" "${1}status/${2}")
    sleep 0.1
    state=$(echo $status | jq -e ".state")
    echo "Status: $status"
done

# If the task suceeded, process subtasks
if [[ $state =~ SUCCESS ]]; then
    subtasks=$(echo $status | jq -r ".output.tasks")
    for task in ${subtasks}; do
        if [ $task != null ] && [ ! -z $task ]; then
            # Recursively call script on subtask
            bash ./poll_webserver_update_region_celery.sh ${1} ${task} ${3}
            # If subtask exited with 1, exit this script with 1 (failure)
            if [[ $? =~ 1 ]]; then
                exit 1
            fi
        fi
    done
    # If all subtasks did not fail, exit script with 0 (success)
    exit 0
else
    # If task failed, exit script with 1 (failure)
    exit 1
fi
