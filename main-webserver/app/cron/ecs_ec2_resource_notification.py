import boto3
import os
import slack
import uuid

from datetime import datetime
from datetime import date

icons = {
    "OLD COMMIT": ":red_circle:",  # commit hash for resource is old - should be deleted
    "OVERDUE": ":red_circle:",  # resource uptime has been greater than 2 weeks - should be deleted or checked
    "CREATED ON TEST": ":red_circle:",  # resource has been created on test - should be deleted
    "CURRENT COMMIT": ":large_green_circle:",  # commit matches current commit hash - ignore resource
    "IGNORE TIME": ":large_green_circle:",  # resouce uptime is < 2 weeks - ignore resource
    "NO COMMIT TAG": ":large_yellow_circle:",  # resources does not contain a commit tag - check resource to make sure if in use
}

regions = [
    "us-east-1",
    "us-east-2",
    "us-west-1",
    "us-west-2",
    "ca-central-1",
    "eu-west-1",
    "eu-central-1",
]

branches = ["steveli/aws-resource-tagging", "staging", "prod"]


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


def read_tags(tags, commit, branch, resource, region, instance_id):
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
    found_name = False
    target_branches = ["dev", "staging", "prod"]
    tag_branch = ""
    tag_commit = ""
    test = ""
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
    launch_time = timestamp.date()

    different = today - launch_time

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
    cluster_arns = client.list_clusters()["clusterArns"]

    response = client.describe_clusters(clusters=cluster_arns)
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
        flag = False

        clusterName = c["clusterName"]
        clusterArn = c["clusterArn"]

        tags = get_tags(clusterArn, region)

        branch, commit, name = read_tags(tags, commit, branch, "ECS", region, "")

        line = f"`{clusterName}`"

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

            tag_branch = ""
            tag_commit = ""
            name = ""

            launch_time = instance["LaunchTime"]
            instance_id = instance["InstanceId"]

            launch_status, days = compare_timestamps(launch_time)
            launch_icon = icons[launch_status]
            if "Tags" in instance:
                tag_branch, tag_commit, name = read_tags(
                    instance["Tags"], commit, branch, "EC2", region, instance_id
                )

            if branch == tag_branch:
                line = f"     • `{name}`"
                message += f"{line} \n"
                message += f"          • id: `{instance_id}` \n"
                message += f"          • Branch: `{tag_branch}` \n"
                message += f"          • Commit: `{tag_commit}` \n"

    return message


if __name__ == "__main__":
    # token = os.environ.get("SLACK_BOT_OAUTH_TOKEN")
    # commit = os.environ.get("HEROKU_SLUG_COMMIT")
    # commit = commit[0:7]
    # branch = os.environ.get("BRANCH")
    token = "xoxb-824878087478-1738745217397-gwRk3we5JOq5Gq7RHceFjBYA"
    commit = "1234"
    client = slack.WebClient(token=token)
    for region in regions:
        blocks = [
            {
                "type": "section",
                "text": {
                    "type": "mrkdwn",
                    "text": "*EC2 Instances in:* _{}_ ".format(region),
                },
            },
        ]

        for branch in branches:
            blocks.extend(
                [
                    {
                        "type": "section",
                        "text": {
                            "type": "mrkdwn",
                            "text": "*Status for branch:* `{}` \n".format(branch),
                        },
                    },
                ]
            )

            print(region, branch)

            instances = flag_instances(region, commit, branch)
            if len(instances) > 0:
                blocks[1]["text"]["text"] += instances

                client.chat_postMessage(channel="U01J21MUCMS", blocks=blocks)
