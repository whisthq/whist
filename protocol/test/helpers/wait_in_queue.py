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
        sys.exit(-1)

    while True:
        queue_length = count_runs_to_prioritize(workflow, github_run_id)
        if queue_length > 0:
            print(
                f"Waiting for our turn to use the reusable instances ({queue_length} runs ahead of us)..."
            )
            time.sleep(15)
        else:
            break

    print("Done waiting!")
    sys.exit(0)
