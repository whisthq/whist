#!/usr/bin/env python3

import os, sys, time
from github import Github

from helpers.common.git_tools import (
    get_workflow_handle,
    count_runs_to_prioritize,
)

from helpers.common.constants import running_in_ci, github_run_id
from helpers.common.timestamps_and_exit_tools import exit_with_error

# This CI script waits until all instances of the current workflow which were created beforehand have completed.
# In case other workflow runs with the exact same creation date were found, the one with the github runner ID with
# the lowest integer value takes precedence


if __name__ == "__main__":
    if not running_in_ci:
        exit_with_error("Error: Cannot run this script outside of CI!")

    workflow = get_workflow_handle()
    if not github_run_id or not workflow:
        exit_with_error("Invalid github run id or workflow handle!")

    while True:
        queue_length = count_runs_to_prioritize(workflow, github_run_id)
        if queue_length > 4:
            # with 5 workflows ahead of us, we'd need to wait ~2.5h if there is no error. So it is better to use new instances
            print(
                "There are too many prioritized runs ahead of us, so we use new instances and run in parallel instead!"
            )
            git_env_file = os.getenv("GITHUB_ENV")
            if not git_env_file or not os.path.isfile(git_env_file or ""):
                exit_with_error("Error, could not access the GITHUB_ENV file!")
            with open(git_env_file, "a") as env_file:
                env_file.write("SERVER_INSTANCE_ID=\n")
                env_file.write("CLIENT_INSTANCE_ID=\n")
                env_file.write("REGION_NAME=\n")
            break
        elif queue_length > 0:
            print(
                f"Waiting for our turn to use the reusable instances ({queue_length} runs ahead of us)..."
            )
            time.sleep(15)
        else:
            break

    print("Done waiting!")
    sys.exit(0)
