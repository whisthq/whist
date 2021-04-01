import boto3
import os
import uuid
import sys


def read_tags(tags, resource):
    """
    Reads tags for a given resource, either EC2 or ECS and returns tags

    Args:
        tags (arr): array of key value pairs
        resource (str): current aws resource, either EC2 or ECS

    Returns:
        tuple: the branch, commit, and name of the resource
    """
    tag_branch = ""
    tag_commit = ""
    name = ""
    test = "False"
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

    return tag_branch, tag_commit, name, test == "True"


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
    test_count = 0
    user_count = 0
    message = ""
    for r in reservations:
        instances = r["Instances"]
        for instance in instances:

            tag_branch = ""
            tag_commit = ""
            name = ""
            test = False

            instance_id = instance["InstanceId"]
            state = instance["State"]["Name"]

            if "Tags" in instance:
                tag_branch, tag_commit, name, test = read_tags(instance["Tags"], "EC2")

            if branch == tag_branch:
                if test:
                    test_count += 1
                else:
                    user_count += 1

                line = f"          - \\`{name}\\`"
                message += f"{line} \n"

    return message, test_count, user_count


if __name__ == "__main__":

    region = sys.argv[1]
    branch = sys.argv[2]

    instances, test, user = flag_instances(region, branch)
    if len(instances) > 0:
        instances = f"     *Summary:* {test} test instances, {user} user instances \n {instances}"
        print(instances)
