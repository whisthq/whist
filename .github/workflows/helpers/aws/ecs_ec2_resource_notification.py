import boto3
import os
import uuid
import sys


def read_tags(tags, resource):
    """
    Reads tags for a given resource, either EC2 or ECS and returns status codes

    Args:
        tags (arr): array of key value pairs
        commit (str): current commit hash
        branch (str): current branch
        resource (str): current aws resource, either EC2 or ECS

    Returns:
        str: status code, either ignore commit, old commit, current commit, or create on test
    """
    tag_branch = ""
    tag_commit = ""
    name = ""
    key = "Key" if resource == "EC2" else "key"
    value = "Value" if resource == "EC2" else "value"
    for tag in tags:
        if tag[key] == "created_on_test":
            test = tag[value]
        if tag[key] == "git_branch":
            tag_branch = tag[value]
        if tag[key] == "git_commit":
            tag_commit = tag[value]
        if tag[key] == "Name":
            name = tag[value]

    return tag_branch, tag_commit, name


def get_all_instances(region):
    """
    Gets all instances from a given region

    Args:
        region (str): current region

    Returns:
        arr: array containing all instances from the given region
    """
    client = boto3.client("ec2", region_name=region)
    response = client.describe_instances()

    return response["Reservations"]


def flag_instances(region, branch):
    """
    Flags all the instances in a given region

    Args:
        region (str): current region
        commit (str): current commit hash
        branch (str): current branch

    Returns:
        str: message to be sent to slack containing all flagged instances
    """
    reservations = get_all_instances(region)
    message = ""
    for r in reservations:
        instances = r["Instances"]
        for instance in instances:

            tag_branch = ""
            tag_commit = ""
            name = ""

            instance_id = instance["InstanceId"]
            state = instance["State"]["Name"]

            if "Tags" in instance:
                tag_branch, tag_commit, name = read_tags(instance["Tags"], "EC2")

            if branch == tag_branch:
                line = f"     - \\`{name}\\`"
                message += f"{line} \n"
                message += f"          - id: \\`{instance_id}\\` \n"
                message += f"          - Branch: \\`{tag_branch}\\` \n"
                message += f"          - Commit: \\`{tag_commit}\\` \n"
                message += f"          - State: \\`{state}\\` \n"

    return message


if __name__ == "__main__":

    region = sys.argv[1]
    branch = sys.argv[2]

    instances = flag_instances(region, branch)
    print(instances)
