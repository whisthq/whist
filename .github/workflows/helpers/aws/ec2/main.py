import boto3
import os
import pandas as pd
import slack
import sys

from datetime import date


icons = {
    "OLD COMMIT": ":red_circle:",
    "OVERDUE": ":red_circle:",
    "CREATED_ON_TEST": ":red_circle:",
    "CURRENT COMMIT": ":large_green_circle:",
    "IGNORE TIME": ":large_yellow_circle:",
    "IGNORE COMMIT": ":large_yellow_circle:",
}


def get_tags(arn, region):
    """
    Gets tags for a cluster with given arn. Have to call tags separately
    because descrbe_clusters doesn't seem to return valid tags.

    Args:
        arn (str): arn of a cluster
        region (str): current region

    Returns:
        array: An array of key value pairs representing the tags of a given cluster
    """
    client = boto3.client("ecs", region_name=region)

    response = client.list_tags_for_resource(resourceArn=arn)
    return response["tags"]


def read_tags(tags, commit, branch, resource):
    """
    Reads tags for a given resource, either EC2 or ECS and returns status codes

    Args:
        tags (arr): array of key value pairs
        commit (str): current commit hash
        branch (str): current branch
        resource (str): current aws resource, either EC2 or ECS

    Returns:
        str: status code, either ignore commit, old commit, current ocmmit, or create on test
    """
    target_branches = ["dev", "staging", "master"]
    tag_branch = ""
    tag_commit = ""
    test = ""
    key = "Key" if resource == "EC2" else "key"
    value = "Value" if resource == "EC2" else "value"
    for t in tags:
        if t[key] == "created_on_test":
            test = t[value]
        if t[key] == "git_branch":
            tag_branch = t[value]
        if t[key] == "git_commit":
            tag_commit = t[value]

    if test == "True":
        return "CREATED ON TEST"
    if branch in target_branches and tag_branch == branch:
        return "OLD COMMIT" if tag_commit != commit else "CURRENT COMMIT"

    return "IGNORE COMMIT"


def compare_timestamps(timestamp):
    """
    Compares a given timestamp with the current date and returns whether
    the timestamp is more than 2 weeks old

    Args:
        timestamp (datetime): datetime object representing the timestamp an EC2 instance was created

    Returns:
        str: a String representing a status code, either overdue or ignore
    """
    today = date.today()
    launchTime = timestamp.date()

    different = today - launchTime

    return "OVERDUE" if different.days >= 14 else "IGNORE TIME", different.days


def get_all_clusters(region):
    """
    Gets all clusters from a given region

    Args:
        region (str): string representing the given region

    Returns:
        array: an array of clusters from the given region
    """
    client = boto3.client("ecs", region_name=region)
    clusterArns = client.list_clusters()["clusterArns"]

    response = client.describe_clusters(clusters=clusterArns)
    return response["clusters"]


def flag_clusters(region, commit, branch):
    """
    Flags all the clusters in a given region

    Args:
        region (str): current region
        commit (str): current commit hash
        branch (str): current branch

    Returns:
        str: message to be sent to slack containing all flagged clusters
    """
    clusters = get_all_clusters(region)
    message = ""

    for c in clusters:
        clusterName = c["clusterName"]
        clusterArn = c["clusterArn"]
        tags = get_tags(clusterArn, region)
        flag_status = read_tags(tags, commit, branch, "ECS")
        icon = icons[flag_status]
        if icon == ":red_circle:":
            message += f"• `{clusterName}` - {flag_status} - {icon} \n"

    return message


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


def flag_instances(region, commit, branch):
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
            tag_status = ""
            launchTime = instance["LaunchTime"]
            instance_id = instance["InstanceId"]
            launch_status, days = compare_timestamps(launchTime)
            launch_icon = icons[launch_status]

            if "Tags" in instance:
                tag_status = read_tags(instance["Tags"], commit, branch, "EC2")
                tag_icon = icons[tag_status]

            if len(tag_status) == 0:
                tag_status = "NO TAGS"
                tag_icon = icons["IGNORE COMMIT"]

            if launch_icon == ":red_circle:" or tag_icon == ":red_circle:":
                message += f"• `{instance_id}` - {tag_status} - {tag_icon} - {launch_status} - uptime: {days} days - {launch_icon} \n"

    return message


if __name__ == "__main__":
    region = os.environ.get("AWS_REGION")
    resource = os.environ.get("AWS_RESOURCE")
    token = os.environ.get("SLACK_EC2_BOT_TOKEN")
    commit = sys.argv[1][0:7]
    branch = sys.argv[2]

    print(branch)
    print(commit)

    # slack message formatter
    blocks = [
        {
            "type": "section",
            "text": {
                "type": "mrkdwn",
                "text": "*{} Status for: {}*".format(resource, region),
            },
        },
        {
            "type": "section",
            "text": {
                "type": "mrkdwn",
                "text": "• Schedule meetings \n ",
            },
        },
    ]

    message = ""

    if resource == "EC2":
        message = flag_instances(region, commit, branch)

    elif resource == "ECS":
        message = flag_clusters(region, commit, branch)

    client = slack.WebClient(token=token)
    blocks[1]["text"]["text"] = message

    if len(message) > 0:
        client.chat_postMessage(channel="U01J21MUCMS", blocks=blocks)