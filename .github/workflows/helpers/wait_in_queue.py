#!/usr/bin/env python3

import os, sys, time
from github import Github

GITHUB_RUN_ID_LEN = 10

if __name__ == "__main__":
    github_run_id = os.getenv("GITHUB_RUN_ID")
    if not github_run_id or len(github_run_id) != 10:
        running_in_ci = os.getenv("CI") == "true"
        if running_in_ci:
            print("We're not in CI! No waiting needed.")
            sys.exit(0)
        else:
            print("Error, could not get the run id!")
            sys.exit(-1)
    github_run_id = int(github_run_id)
    git_token = os.getenv("GITHUB_TOKEN") or ""
    if len(git_token) != 40:
        print("Either the GITHUB_TOKEN env is not set, or a token of the wrong length was passed.")
        sys.exit(-1)
    git_client = Github(git_token)
    if not git_client:
        print("Could not obtain Github client!")
        sys.exit(-1)
    repo = git_client.get_repo("whisthq/whist")
    if not repo:
        print("Could not access the whisthq/whist repository!")
        sys.exit(-1)
    workflow = repo.get_workflow("protocol-e2e-streaming-testing.yml")
    if not workflow:
        print("Could not access the protocol-e2e-streaming-testing.yml workflow!")
        sys.exit(-1)
    
    # possible github statuses are: "queued", "in_progress", "completed"
    running_states = ["queued", "in_progress"]
    while True:
        running_workflows = [
            run for status in running_states for run in workflow.get_runs(status=status)
        ]
        
        this_workflow_creation_time = None
        found = False
        for w in running_workflows:
            if w.id == github_run_id:
                this_workflow_creation_time = w.created_at
                found = True
                running_workflows = [
                    w_good
                    for w_good in running_workflows
                    if w_good.id != github_run_id
                    and (
                        (w_good.created_at < this_workflow_creation_time)
                        or (
                            w_good.created_at <= this_workflow_creation_time
                            and w_good.id < github_run_id
                        )
                    )
                ]
                break
        assert found == True
        queue_length = len(running_workflows)
        if queue_length > 0:
            print(
                f"Waiting for our turn to use the reusable instances ({queue_length} runs ahead of us)..."
            )
            time.sleep(15)
        else:
            break

    print("Done waiting!")
    sys.exit(0)
