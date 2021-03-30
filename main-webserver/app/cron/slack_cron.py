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


def add_name(commit, branch, region, instance_id, test):
    """
    Adds a name tag for a given instance

    Args:
        commit (str): commit hash
        branch (str): current branch name
        region (str): region name
        instance_id (str): given instance_id
        test (bool): if instance was created on test or not
    """
    name = f"{'test-' if test else ''}<{branch}>-<{commit}>-{uuid.uuid4()}"
    client = boto3.client("ec2", region_name=region)
    client.create_tags(Resources=[instance_id], Tags=[{"Key": "Name", "Value": name}])

    return name


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
            found_name = True

    if resource == "EC2" and not found_name:
        name = add_name(tag_commit, tag_branch, region, instance_id, test == "True")

    if test == "True":
        return "CREATED ON TEST", tag_branch, tag_commit, name
    if branch in target_branches and tag_branch == branch:
        return (
            ("OLD COMMIT", tag_branch, tag_commit, name)
            if tag_commit != commit
            else ("CURRENT COMMIT", tag_branch, tag_commit, name)
        )

    return ("NO COMMIT TAG", tag_branch, tag_commit, name)


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

        git_status, branch, commit, name = read_tags(tags, commit, branch, "ECS", region, "")
        git_icon = icons[git_status]

        line = f"`{clusterName}`"

        if git_icon == ":red_circle:":
            flag = True
            line = f"• {git_icon} - {line} - {git_status} \n"
            message += line

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
            flag = False

            tag_branch = ""
            tag_commit = ""
            git_status = ""
            name = ""

            launch_time = instance["LaunchTime"]
            instance_id = instance["InstanceId"]

            launch_status, days = compare_timestamps(launch_time)
            launch_icon = icons[launch_status]

            if "Tags" in instance:
                git_status, tag_branch, tag_commit, name = read_tags(
                    instance["Tags"], commit, branch, "EC2", region, instance_id
                )
                git_icon = icons[git_status]
            else:
                name = add_name(commit, branch, region, instance_id, False)
                git_status = "NO TAGS"
                git_icon = icons["NO COMMIT TAG"]

            line = f"`{name}`"

            if git_icon == ":red_circle:":
                flag = True
                line += f"- {git_status}"

            if launch_icon == ":red_circle:":
                flag = True
                line += f" - {launch_status} - uptime: {days} days "

            if flag:
                line = f"• :red_circle: {line}"
                message += f"{line} \n"
                message += f"          • id: `{instance_id}` \n"

                if len(tag_branch) > 0 and len(tag_commit) > 0:
                    message += f"          • Branch: `{tag_branch}` \n"
                    message += f"          • Commit: `{tag_commit}` \n"

    return message


if __name__ == "__main__":
    # token = os.environ.get("SLACK_BOT_OAUTH_TOKEN")
    # commit = os.environ.get("HEROKU_SLUG_COMMIT")
    # commit = commit[0:7]
    # branch = os.environ.get("BRANCH")
    token = "xoxb-824878087478-1738745217397-gwRk3we5JOq5Gq7RHceFjBYA"
    commit = "01234"
    branch = "asdf"

    # slack message formatter
    for resource in ["EC2", "ECS"]:
        for region in regions:
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
