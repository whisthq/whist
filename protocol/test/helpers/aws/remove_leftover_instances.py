#!/usr/bin/env python3

import os, sys, boto3

from helpers.aws.boto3_tools import terminate_or_stop_aws_instance
from helpers.common.constants import instances_name_tag, running_in_ci, github_run_id
from helpers.common.timestamps_and_exit_tools import printred
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


def stop_ci_reusable_instances():
    client_instance_id = os.getenv("REUSABLE_CLIENT_INSTANCE_ID") or ""
    server_instance_id = os.getenv("REUSABLE_SERVER_INSTANCE_ID") or ""
    region_name = os.getenv("REGION_NAME") or ""
    for instance_id in set([x for x in (client_instance_id, server_instance_id) if len(x) > 0]):
        if region_name == "":
            printred(
                f"Cannot stop reusable instance {instance_id} because REGION_NAME environment variable is not set!"
            )
        else:
            print(f"Stopping instance '{instance_id}' in region '{region_name}' ...")
            terminate_or_stop_aws_instance(boto3client, instance_id, should_terminate=False)


def get_leftover_instances(region_name, leftover_instances_filters):
    boto3client = boto3.client("ec2", region_name=region_name)
    response = boto3client.describe_instances(Filters=leftover_instances_filters)

    leftover_instances = []

    reservations = response.get("Reservations") if response.get("Reservations") else []
    for reservation in reservations:
        instances = reservation.get("Instances") if reservation.get("Instances") else []
        instance_ids = [x["InstanceId"] for x in instances if x["State"]["Name"] != "terminated"]
        leftover_instances = leftover_instances + instance_ids

    return leftover_instances


if __name__ == "__main__":

    if running_in_ci:
        # Stop reusable instances
        print("Stopping CI reusable instances...")
        stop_ci_reusable_instances()

    # Search for any leftover instances with names suggesting they were created by the latest E2E instances
    branch_name = get_whist_branch_name()
    instance_name = f"{instances_name_tag}-{branch_name}"
    name_tag_match = {"Name": "tag:Name", "Values": [instance_name]}
    instance_creator_match = {"Name": "tag:RunID", "Values": [github_run_id]}
    leftover_instances_filters = [name_tag_match, instance_creator_match]

    ec2_region_names = [
        region["RegionName"] for region in boto3.client("ec2").describe_regions()["Regions"]
    ]
    for region_name in ec2_region_names:
        leftover_instances = get_leftover_instances(region_name, leftover_instances_filters)
        for instance_id in leftover_instances:
            print(f"Terminating instance '{instance_id}' in region '{region_name}' ...")
            terminate_or_stop_aws_instance(boto3client, instance_id, should_terminate=True)
