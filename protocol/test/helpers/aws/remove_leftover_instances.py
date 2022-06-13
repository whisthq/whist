#!/usr/bin/env python3

import os, sys, boto3

from helpers.aws.boto3_tools import terminate_or_stop_aws_instance
from helpers.common.constants import instances_name_tag, running_in_ci, github_run_id
from helpers.common.git_tools import get_whist_branch_name

# Before exiting, the streaming_e2e_tester.py script stops/terminates all EC2 instances
# (unless the `--leave-instances-on` flag is set to 'true') used in the E2E test.
# The script terminates all newly-created instances, and stops all pre-existing instances that were re-used.
#
# However, if streaming_e2e_tester.py crashes, it will likely do so before completing the cleanup.
# To handle this scenario, we make the E2E script log a todolist of all cleanup actions that are
# required. The list is saved to a file called `instances_to_clean.txt` and located in the current
# working directory. Upon successful completion of the cleanup, the E2E will delete the file.
#
# If the E2E crashes, the file will still exist after the E2E has exited, and this script
# will follow the todolist and complete the cleanup.

if __name__ == "__main__":

    filepath = os.path.join("instances_to_remove.txt")

    # If the `instances_to_clean.txt` todolist does not exist, streaming_e2e_tester.py has already
    # completed the cleanup, and we are done.
    removed_instances = []
    if os.path.exists(filepath) and os.path.isfile(filepath):
        todo_list = open(filepath, "r")

        # Follow the todolist and stop/terminate any leftover instance.
        for line in todo_list.readlines():
            cleanup_action, aws_region_name, instance_id = line.strip().split()
            # Connect to the E2 console
            boto3client = boto3.client("ec2", region_name=aws_region_name)
            removed_instances.append(instance_id)
            # Check if we need to stop or terminate the instance
            if cleanup_action == "terminate":
                print(f"Terminating instance '{instance_id}' in region '{aws_region_name}' ...")
                terminate_or_stop_aws_instance(boto3client, instance_id, should_terminate=True)
            elif cleanup_action == "stop":
                print(f"Stopping instance '{instance_id}' in region '{aws_region_name}' ...")
                terminate_or_stop_aws_instance(boto3client, instance_id, should_terminate=False)

        # Close and delete the todolist upon completion of the cleanup
        todo_list.close()
        os.remove(filepath)

    # Also search for any leftover instances with names suggesting they were created by the latest E2E instance.
    # On CI, we terminate any of these instances automatically. On a normal machine, we ask the user for a confirmation
    ec2_region_names = [
        region["RegionName"] for region in boto3.client("ec2").describe_regions()["Regions"]
    ]
    for region_name in ec2_region_names:
        branch_name = get_whist_branch_name()
        instance_name = f"{instances_name_tag}-{branch_name}"
        name_tag_match = {"Name": "tag:Name", "Values": [instance_name]}
        instance_creator_match = {"Name": "tag:RunID", "Values": [github_run_id]}
        boto3client = boto3.client("ec2", region_name=region_name)
        response = boto3client.describe_instances(Filters=[name_tag_match, instance_creator_match])

        leftover_instances = []

        reservations = response.get("Reservations") if response.get("Reservations") else []
        for reservation in reservations:
            instances = reservation.get("Instances") if reservation.get("Instances") else []
            instance_ids = [
                x["InstanceId"] for x in instances if x["State"]["Name"] != "terminated"
            ]
            leftover_instances = leftover_instances + instance_ids

        for instance_id in leftover_instances:
            if instance_id in removed_instances:
                continue

            if running_in_ci:
                print(f"Terminating instance '{instance_id}' in region '{region_name}' ...")
                terminate_or_stop_aws_instance(boto3client, instance_id, should_terminate=True)
            else:
                print(
                    f"Found potentially leftover E2E instance '{instance_id}' in region '{region_name}'. What do you want to do?"
                )
                action = input("Press 1 to terminate it, 2 to stop it, 3 for no action: ")
                print(f"Got: {action}")
                if action == "1":
                    print(f"Terminating instance '{instance_id}' in region '{region_name}' ...")
                    terminate_or_stop_aws_instance(boto3client, instance_id, should_terminate=True)
                elif action == "2":
                    print(f"Stopping instance '{instance_id}' in region '{region_name}' ...")
                    terminate_or_stop_aws_instance(boto3client, instance_id, should_terminate=False)
                else:
                    print(f"Ignoring instance '{instance_id}' in region '{region_name}'")
