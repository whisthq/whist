#!/usr/bin/env python3

import os, sys, boto3

from helpers.aws.boto3_tools import terminate_or_stop_aws_instance
from helpers.common.constants import instances_name_tag, running_in_ci, github_run_id
from helpers.common.timestamps_and_exit_tools import printgreen
from helpers.common.git_tools import (
    get_whist_branch_name,
    get_workflow_handle,
    count_runs_to_prioritize,
)

# Before exiting, the streaming_e2e_tester.py script stops/terminates all EC2 instances
# (unless the `--leave-instances-on` flag is set to 'true') used in the E2E test.
# The script terminates all newly-created instances, and stops all pre-existing instances that were re-used.
#
# However, if streaming_e2e_tester.py crashes, it will likely do so before completing the cleanup.
# To handle this scenario, this additional script will search for any leftover instances, and terminate
# or stop them (depending on whether they are new instances or reused ones).


def stop_ci_reusable_instances():
    """
    Check if the REUSABLE_CLIENT_INSTANCE_ID or REUSABLE_SERVER_INSTANCE_ID environment variables are set,
    and, if so, use the instance ID(s) to stop the reusable instances. If the REGION_NAME env is not set,
    but either REUSABLE_CLIENT_INSTANCE_ID or REUSABLE_SERVER_INSTANCE_ID are set, this function prints
    a warning and does not stop any instance.

    Args:

    Returns:
        None
    """
    region_name = os.getenv("REUSABLE_INSTANCES_REGION_NAME") or ""
    if region_name == "":
        print(
            f"Cannot stop reusable instance(s) because REUSABLE_INSTANCES_REGION_NAME environment variable is not set!"
        )
        return

    # Check if there are other runs with priority access to the shared instances. If that is the case,
    # we are not authorized to stop the reusable instances. The run with 1° priority will take care of
    # stopping them once done
    workflow = get_workflow_handle()
    if not github_run_id or not workflow:
        sys.exit(-1)
    if count_runs_to_prioritize(workflow, github_run_id) > 0:
        printgreen(
            "We do not stop the reusable instances because other workflows are using them before us."
        )
        return

    boto3client = boto3.client("ec2", region_name=region_name)

    client_instance_id = os.getenv("REUSABLE_CLIENT_INSTANCE_ID") or ""
    server_instance_id = os.getenv("REUSABLE_SERVER_INSTANCE_ID") or ""

    for instance_id in set([x for x in (client_instance_id, server_instance_id) if len(x) > 0]):
        print(f"Stopping instance '{instance_id}' in region '{region_name}' ...")
        terminate_or_stop_aws_instance(boto3client, instance_id, should_terminate=False)


def get_leftover_instances(boto3client, region_name, leftover_instances_filters):
    """
    Given a region name and a set of AWS filters (e.g. instance names, tags, etc...), fetch the list of instances
    matching all the filters, and return the instance IDs

    Args:
        boto3client (botocore.client): The Boto3 client to use to talk to the Amazon E2 service
        region_name (str): The AWS region where we need to look for instances with characteristics matching the filters
        leftover_instances_filters (list):  The list of filters to be used to query the instances. Only instances matching
                                            all filters will be included in the result.

    Returns:
        leftover_instances (list): List of instance IDs of the instances that were found to match the provided filters.
    """
    leftover_instances = []

    response = boto3client.describe_instances(Filters=leftover_instances_filters)
    # The describe_instances function organizes the results in a list of dictionaries called "Reservations", so  we
    # have to iterate through all entries to obtain all instance IDs matching the filters.
    reservations = response.get("Reservations") or []
    for reservation in reservations:
        instances = reservation.get("Instances") or []
        instance_ids = [x["InstanceId"] for x in instances if x["State"]["Name"] != "terminated"]
        leftover_instances = leftover_instances + instance_ids

    return leftover_instances


if __name__ == "__main__":

    # 1. Stop the reusable instances, if they were used in the workflow running this script
    if running_in_ci:
        print("Stopping CI reusable instances...")
        stop_ci_reusable_instances()

    # 2. Terminate all instances that were created anew by the workflow (or E2E scripts) that last ran on the current machine.

    # Search for instances created by the current github runner (or by any personal machine if the script is run outside of CI)
    # to test the current branch of the Whist repository
    branch_name = get_whist_branch_name()
    instance_name = f"{instances_name_tag}-{branch_name}"
    name_tag_match = {"Name": "tag:Name", "Values": [instance_name]}
    instance_creator_match = {"Name": "tag:RunID", "Values": [github_run_id]}
    leftover_instances_filters = [name_tag_match, instance_creator_match]

    # Get list of all available EC2 regions
    ec2_region_names = [
        region["RegionName"] for region in boto3.client("ec2").describe_regions()["Regions"]
    ]

    # Search for leftover instances in each region, and terminate the ones matching the criteria above
    for region_name in ec2_region_names:
        boto3client = boto3.client("ec2", region_name=region_name)
        leftover_instances = get_leftover_instances(
            boto3client, region_name, leftover_instances_filters
        )
        for instance_id in leftover_instances:
            print(f"Terminating instance '{instance_id}' in region '{region_name}' ...")
            terminate_or_stop_aws_instance(boto3client, instance_id, should_terminate=True)
