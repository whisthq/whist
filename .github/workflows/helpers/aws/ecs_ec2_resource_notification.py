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
        tuple: the branch, commit, name of the resource, and whether it was created on test
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


def flag_instances(region):
    """
    Flags all the instances in a given region

    Args:
        region (str): current region

    Returns:
        map: map of all branches with associated instances
    """
    reservations = get_all_instances(region)
    branch_map = {}
    test_instances = 0
    user_instances = 0
    for r in reservations:
        instances = r["Instances"]
        for instance in instances:

            tag_branch = ""
            tag_commit = ""
            name = ""
            test = False

            if "Tags" in instance:
                tag_branch, tag_commit, name, test = read_tags(instance["Tags"], "EC2")

            if len(tag_branch) > 0:
                # [ user instances, test instances ]
                if tag_branch in branch_map:
                    if test:
                        branch_map[tag_branch][1] += 1
                    else:
                        branch_map[tag_branch][0] += 1
                else:
                    branch_map[tag_branch] = [0, 1] if test else [1, 0]

    return branch_map


if __name__ == "__main__":

    region = sys.argv[1]

    branches = flag_instances(region)
    if len(branches) > 0:
        message = ""
        for branch in branches:
            message += f"     *Branch* \\`{branch}\\`: {branches[branch][0]} *user instances* - {branches[branch][1]} *test instances*\n"
        print(message)
