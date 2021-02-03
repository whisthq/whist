# ${1} the webserver url
# ${2} the task ID in question

state=PENDING

# poll for task to finish
while [[ $state =~ PENDING ]]; do
    status=$(curl -L -X GET "${1}status/${2}")
    state=$(echo $status | jq ".state")
done

# Get subtasks of completed task. If task failed or has no subtasks, this is null
subtasks=$(echo $status | jq ".output.tasks" | sed 's/[][ \"]//g' | sed 's/,/ /g')

# If the task suceeded, process subtasks
if [[ $state =~ SUCCESS ]]; then
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
