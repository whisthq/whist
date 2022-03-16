#!/usr/bin/env python3

import os
import sys
import boto3
from e2e_helpers.boto3.boto3_tools import terminate_or_stop_aws_instance

# Before exiting, the streaming_e2e_tester.py script stops/terminates all EC2 instances
# (unless the `--leave-instances-on` flag is set to 'true') used in the E2E test.
# The script terminates all newly-created instances, and stops all pre-existing instances that were re-used.

# However, if streaming_e2e_tester.py crashes, it will likely do so before completing the cleanup.
# To handle this scenario, we make the E2E script log a TODO-list of all cleanup actions that are
# required. The list is saved to a file called `instances_to_clean.txt` and located in the current
# working directory. Upon successful completion of the cleanup, the E2E will delete the file.

# If the E2E crashes, the file will still exist after the E2E has exited, and this script
# will follow the TODO-list and complete the cleanup.

if __name__ == "__main__":

    filepath = os.path.join("instances_to_remove.txt")

    # If the `instances_to_clean.txt` TODO-list does not exist, streaming_e2e_tester.py has already completed the cleanup, and we are done.
    if not os.path.exists(filepath) or not os.path.isfile(filepath):
        print("No leftover instances need to be stopped or terminated")
        sys.exit()

    todo_list = open(filepath, "r")

    # Follow the TODO-list and stop/terminate any leftover instance.
    for line in todo_list.readlines():
        cleanup_action, aws_region_name, instance_id = line.strip().split()
        # Connect to the E2 console
        boto3client = boto3.client("ec2", region_name=aws_region_name)
        # Check if we need to stop or terminate the instance
        if cleanup_action == "terminate":
            print(f"Terminating instance '{instance_id}' in region '{aws_region_name}' ...")
            terminate_or_stop_aws_instance(boto3client, instance_id, should_terminate=True)
        elif cleanup_action == "stop":
            print(f"Stopping instance '{instance_id}' in region '{aws_region_name}' ...")
            terminate_or_stop_aws_instance(boto3client, instance_id, should_terminate=False)

    # Close and delete the TODO-list upon completion of the cleanup
    todo_list.close()
    os.remove(filepath)
