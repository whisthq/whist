#!/bin/bash

# Filter to only job-name, if a specific job-name is given
# But if the job-name is "all", then don't filter anything
if [[ -n "$JOB_NAME" && "$JOB_NAME" != "all" ]]; then
  JOB_JSON=$(echo "$JOB_JSON" | jq "with_entries(select(.key == \"$JOB_NAME\"))")
fi

# Loop over JSON keys
for key in $(echo "$JOB_JSON" | jq -r 'keys[]'); do
  # Get list of dependency paths for this job (ie, job "$key")
  # Replacing \n and " with spaces so that paths can be passed as arguments to check-paths-in-diff.sh
  DEPENDENCY_PATHS=$(echo "$JOB_JSON" | jq ".[\"$key\"][]" | tr '\n' ' ' | tr '"' ' ')
  # Get the JSON value at that key, and then pipe it into check-paths-in-diff.sh
  # This will succeed if no dependency paths are given, or if at least
  # one file in a dependency path has been changed
  if ! git diff --name-only HEAD~10 | ./helpers/deploy/check_paths_in_diff.sh $DEPENDENCY_PATHS; then
    JOB_JSON=$(echo "$JOB_JSON" | jq "with_entries(select(.key != \"$key\"))")
  fi
done

# Print space separated list of jobs to run
echo "$JOB_JSON" | jq -r 'keys[]' | tr '\n' ' '

