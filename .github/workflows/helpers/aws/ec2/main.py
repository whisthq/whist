import boto3
import os
import pandas as pd
import slack
import sys

from datetime import date


icons = {
    "OLD COMMIT": ":red_circle:",
    "OVERDUE": ":red_circle:",
    "CURRENT COMMIT": ":large_green_circle:",
    "IGNORE TIME": ":large_yellow_circle:",
    "IGNORE COMMIT": ":large_yellow_circle:",
}


def get_tags(arn, region):
    client = boto3.client("ecs", region_name=region)

    response = client.list_tags_for_resource(resourceArn=arn)
    return response["tags"]


def read_tags(tags, commit, branch, resource):
    target_branches = ["steveli/aws-resource-tagging", "staging", "master"]
    tag_branch = ""
    tag_commit = ""
    key = "Key" if resource == "EC2" else "key"
    value = "Value" if resource == "EC2" else "value"
    for t in tags:
        if t[key] == "git_branch":
            tag_branch = t[value]
        if t[key] == "git_commit":
            tag_commit = t[value]
    # print(branch, tag_branch)
    if branch in target_branches and tag_branch == branch:
        return "OLD COMMIT" if tag_commit != commit else "CURRENT COMMIT"

    return "IGNORE COMMIT"


def compare_timestamps(timestamp):
    today = date.today()
    launchTime = timestamp.date()

    different = today - launchTime

    return "OVERDUE" if different.days >= 14 else "IGNORE TIME", different.days


def get_all_clusters(region):
    client = boto3.client("ecs", region_name=region)
    clusterArns = client.list_clusters()["clusterArns"]

    response = client.describe_clusters(clusters=clusterArns)
    return response["clusters"]


def flag_clusters(region, commit, branch):
    clusters = get_all_clusters(region)
    message = ""

    for c in clusters:
        clusterName = c["clusterName"]
        clusterArn = c["clusterArn"]
        tags = get_tags(clusterArn, region)
        flag_status = read_tags(tags, commit, branch, "ECS")
        icon = icons[flag_status]
        if icon == ":red_circle:":
            message += f"• {clusterName} - {flag_status} - {icon} \n"

    return message


def get_all_instances(region):
    client = boto3.client("ec2", region_name=region)
    response = client.describe_instances()

    return response["Reservations"]


def flag_instances(region, commit, branch):
    reservations = get_all_instances(region)
    message = ""
    for r in reservations:
        instances = r["Instances"]
        for instance in instances:
            tag_status = ""
            launchTime = instance["LaunchTime"]
            instance_id = instance["InstanceId"]
            launch_status, days = compare_timestamps(launchTime)
            if "Tags" in instance:
                tag_status = read_tags(instance["Tags"], commit, branch, "EC2")
                launch_icon = icons[launch_status]
                tag_icon = icons[tag_status]

            if len(tag_status) == 0:
                tag_status = "NO TAGS"
                tag_icon = icons["IGNORE COMMIT"]

            if launch_icon == ":red_circle:" or tag_icon == ":red_circle:":
                message += f"• {instance_id} - {tag_status} - {tag_icon} - {launch_status} - uptime: {days} days - {launch_icon} \n"

    return message


if __name__ == "__main__":
    region = os.environ.get("AWS_REGION")
    resource = os.environ.get("AWS_RESOURCE")
    token = os.environ.get("SLACK_EC2_BOT_TOKEN")
    commit = sys.argv[1][0:7]
    branch = sys.argv[2]

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
        message = flag_instances(region, "asdf", "steveli/aws-resource-tagging")

    elif resource == "ECS":
        message = flag_clusters(region, "asdf", "steveli/aws-resource-tagging")

    client = slack.WebClient(token=token)
    blocks[1]["text"]["text"] = (
        message if len(message) > 0 else "No Instances available"
    )
    client.chat_postMessage(channel="U01J21MUCMS", blocks=blocks)