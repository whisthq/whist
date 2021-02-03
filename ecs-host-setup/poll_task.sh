# ${1} the webserver url
# ${2} the task ID in question

# poll for task to finish
state=PENDING
while [[ $state =~ PENDING ]]; do
    echo "${1}status/${2}"
    status=$(curl -L -X GET "${1}status/${2}")
    echo "status: $status"
    state=$(echo $status | jq -e ".state")
done

# If the task suceeded, process subtasks
if [[ $state =~ SUCCESS ]]; then
    subtasks=$(echo $status | jq -r ".output.tasks")
    echo "subtasks: $subtasks"
    for task in ${subtasks}; do
        if [ $task != null ] && [ ! -z $task ]; then
            # Recursively call script on subtask
            bash poll_task.sh ${1} ${task}
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
